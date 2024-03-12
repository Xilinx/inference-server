// Copyright 2024 Advanced Micro Devices, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file
 * @brief This example demonstrates how you can use the rocAL backend to run
 * image decoding & preprocessing. Look at the documentation
 * online for discussion around this example.
 */

#include <algorithm>            // for copy, max
#include <array>                // for array
#include <cassert>              // for assert
#include <chrono>               // for duration
#include <cstdint>              // for int64_t
#include <cstdlib>              // for exit, getenv
#include <exception>            // for exception
#include <filesystem>           // for path, oper...
#include <initializer_list>     // for initialize...
#include <iostream>             // for operator<<
#include <memory>               // for allocator
#include <opencv2/core.hpp>     // for CV_32FC3
#include <opencv2/imgproc.hpp>  // for COLOR_BGR2RGB
#include <optional>             // for optional
#include <ratio>                // for milli
#include <string>               // for string
#include <vector>               // for vector

#include "amdinfer/amdinfer.hpp"                       // for InferenceR...
#include "amdinfer/pre_post/image_preprocess.hpp"      // for ImagePrepr...
#include "amdinfer/pre_post/resnet50_postprocess.hpp"  // for resnet50Po...
#include "resnet50.hpp"                                // for Args, pars...

namespace fs = std::filesystem;

using Images = std::vector<std::vector<unsigned char>>;

/**
 * @brief Given a vector of paths to images, load the images and return as raw compressed buffer
 * 
 * @param paths paths to images to preprocess
 * @return Images : Raw encoded buffer
 */
Images loadImages(const std::vector<std::string>& paths) {
  Images imageBuffer;

  for (const auto& path : paths) {
    std::ifstream file(path, std::ios::binary);
    if (file) {
      file.seekg(0, std::ios::end);
      size_t size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<unsigned char> buffer(size);
      if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw amdinfer::file_read_error("Failed to read file: " + path);
      }

      imageBuffer.push_back(std::move(buffer));
    } else {
      std::cerr << "Could not open file at: " << path << std::endl;
    }
  }
  return imageBuffer;
}

/**
 * @brief Construct requests for the inference server from the input buffers. 
 *
 * @param imageBuffer the input image buffer
 * @return std::vector<amdinfer::InferenceRequest>
 */
std::vector<amdinfer::InferenceRequest> constructRequests(const Images& imageBuffer) {
  std::vector<amdinfer::InferenceRequest> requests;
  requests.reserve(imageBuffer.size());

  for (const auto& buffer : imageBuffer) {
    requests.emplace_back();

    const std::initializer_list<int64_t> shape = {static_cast<int64_t>(buffer.size())};
    requests.back().addInputTensor((void*)buffer.data(), shape, amdinfer::DataType::Uint8);
  }

  return requests;
}

/**
 * @brief Load a worker to handle an inference request. The load returns the
 * endpoint you should use for subsequent requests.
 *
 * @param client pointer to a client object
 * @param args the command-line arguments
 * @return std::string
 */
std::string load(const amdinfer::Client* client, const Args& args) {
  // Depending on how the server is compiled, it may or may not have support for
  // a particular backend. This guard checks to make sure the server does
  // support the requested backend. If you already know it's supported, you can
  // skip this check.
  if (!serverHasExtension(client, "rocal")) {
    std::cerr << "rocAL is not enabled. Please recompile with it enabled to "
              << "run this example\n";
    exit(0);
  }

  // Load-time parameters are used to pass one-time information to the batcher
  // and worker as it starts up. Each worker can choose to define its own
  // parameters that it pays attention to. Similarly, the batcher that the
  // worker is using may have its own parameters. Check the documentation to see
  // what may be specified.

  // +load
  amdinfer::ParameterMap parameters;

  // Required: specifies path to the model on the server for it to open
  parameters.put("model", args.path_to_model);
  // Optional: request a particular batch size to be sent to the backend. The
  // server will attempt to coalesce incoming requests into a single batch of
  // this size and pass it all to the backend.
  parameters.put("batch", args.batch_size);
  // Optional: specifies how long the batcher should wait for more requests
  // before sending the batch on
  std::string endpoint = client->workerLoad("rocal", parameters);
  amdinfer::waitUntilModelReady(client, endpoint);
  // -load
  return endpoint;
}

/**
 * @brief The command-line arguments are parsed in two phases. There's the
 * common arguments that are initialized by parseArgs that are shared by all the
 * C++ examples in this directory and then example-specific settings are
 * initialized here
 *
 * @param argc number of arguments
 * @param argv arguments
 * @return Args
 */
Args getArgs(int argc, char** argv) {
  Args args = parseArgs(argc, argv);

  // if the model is not specified and no endpoint flag is passed, attempt to
  // set a default value for the model to use for loading
  if (args.path_to_model.empty() && args.endpoint.empty()) {
    const auto* root_str = std::getenv("AMDINFER_ROOT");
    assert(root_str != nullptr);
    fs::path root{root_str};
    args.path_to_model = root / "examples/resnet50/rocal_pipeline.json";
    args.path_to_image = root / "tests/assets/imagenet-dog.jpg";
  }
  return args;
}

int main(int argc, char* argv[]) {
  try {
    std::cout << "Running the rocAL example in C++\n";

    Args args = getArgs(argc, argv);

    const auto http_port_str = std::to_string(args.http_port);
    const auto server_addr = "http://" + args.ip + ":" + http_port_str;
    amdinfer::HttpClient client{server_addr};

    std::optional<amdinfer::Server> server;
    if (args.wait) {
      // if wait is true, skip ahead to waiting for the server to become ready
    } else if (args.ip == "127.0.0.1" && !client.serverLive()) {
      std::cout << "No server detected. Starting locally...\n";
      server.emplace();
      server.value().startHttp(args.http_port);
    } else if (!client.serverLive()) {
      std::cerr << "Could not connect to server at " << server_addr << "\n";
      return 1;
    } else {
      // the server is reachable so continue on
    }

    std::cout << "Waiting until the server is ready...\n";
    amdinfer::waitUntilServerReady(&client);

    std::string endpoint;
    if (!args.endpoint.empty()) {
      endpoint = args.endpoint;
      if (!client.modelReady(endpoint)) {
        std::cerr << "Model at " << endpoint
                  << " does not exist or isn't ready. Verify the endpoint or "
                     "omit the --endpoint flag to load a new worker\n";
        return 1;
      }
    } else {
      std::cout << "Loading worker...\n";
      endpoint = load(&client, args);
    }

    std::vector<std::string> paths = resolveImagePaths(args.path_to_image);

    Images imageBuffer = loadImages(paths);
    std::vector<amdinfer::InferenceRequest> requests =
      constructRequests(imageBuffer);
    assert(paths.size() == requests.size());
    const auto num_requests = requests.size();

    std::cout << "Preprocessing image...\n";
    auto start = std::chrono::high_resolution_clock::now();

    // +validate:
    // rocal.cpp
    std::vector<amdinfer::InferenceResponse> responses =
      amdinfer::inferAsyncOrdered(&client, endpoint, requests);
    assert(num_requests == responses.size());

    for (auto i = 0U; i < num_requests; ++i) {
      const amdinfer::InferenceResponse& response = responses[i];
      assert(!response.isError());

      std::vector<amdinfer::InferenceResponseOutput> outputs =
        response.getOutputs();
      // for rocAL, we expect a output tensor size = num_requests
      assert(outputs.size() == num_requests);
    }
    // -validate:
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = stop - start;
    std::cout << "Time taken for inference, postprocessing and printing: "
              << duration.count() << " ms\n";

    return 0;
  } catch (const amdinfer::runtime_error& e) {
    std::cerr << e.what() << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
