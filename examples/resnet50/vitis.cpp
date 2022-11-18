// Copyright 2021 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
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
 * @brief This example demonstrates how you can use the XModel backend to run
 * inference on an AMD FPGA with a ResNet50 XModel. Look at the documentation
 * online for discussion around this example.
 */

#include <array>             // for array
#include <cassert>           // for assert
#include <chrono>            // for duration
#include <cstdint>           // for int8_t
#include <cstdlib>           // for exit, getenv
#include <filesystem>        // for path, oper...
#include <initializer_list>  // for initialize...
#include <iostream>          // for operator<<
#include <memory>            // for allocator
#include <opencv2/core.hpp>  // for int8_t
#include <optional>          // for optional
#include <ratio>             // for milli
#include <string>            // for string
#include <vector>            // for vector

// +include:
#include "amdinfer/amdinfer.hpp"
// -include:

#include "amdinfer/pre_post/image_preprocess.hpp"
#include "amdinfer/pre_post/resnet50_postprocess.hpp"
#include "resnet50.hpp"

namespace fs = std::filesystem;

using Images = std::vector<std::vector<int8_t>>;

/**
 * @brief Given a vector of paths to images, preprocess the images and return
 * them
 *
 * @param paths paths to images to preprocess
 * @return Images
 */
Images preprocess(const std::vector<std::string>& paths) {
  const std::array<int8_t, 3> mean{123, 107, 104};
  const std::array<int8_t, 3> std{1, 1, 1};

  // this example uses a custom image preprocessing function. You may use any
  // preprocessing logic or skip it entirely if your input data is already
  // preprocessed.
  amdinfer::pre_post::ImagePreprocessOptions<int8_t, 3> options;
  options.order = amdinfer::pre_post::ImageOrder::NHWC;
  options.mean = mean;
  options.std = std;
  options.normalize = true;
  return amdinfer::pre_post::imagePreprocess(paths, options);
}

/**
 * @brief Postprocess the output data. For ResNet50, this includes performing a
 * softmax to determine the most probable classifications.
 *
 * @param output the output from the inference server
 * @param k number of top categories to return
 * @return std::vector<int> the indices for the top k categories
 */
std::vector<int> postprocess(const amdinfer::InferenceResponseOutput& output,
                             int k) {
  return amdinfer::pre_post::resnet50Postprocess(
    static_cast<const int8_t*>(output.getData()), output.getSize(), k);
}

/**
 * @brief Construct requests for the inference server from the input images. For
 * ResNet50, a valid request includes a single input tensor containing a square
 * image.
 *
 * @param images the input images
 * @param input_size size of the square image in pixels
 * @return std::vector<amdinfer::InferenceRequest>
 */
std::vector<amdinfer::InferenceRequest> constructRequests(const Images& images,
                                                          uint64_t input_size) {
  // +construct request:
  std::vector<amdinfer::InferenceRequest> requests;
  requests.reserve(images.size());

  const std::initializer_list<uint64_t> shape = {input_size, input_size, 3};

  for (const auto& image : images) {
    requests.emplace_back();
    requests.back().addInputTensor((void*)image.data(), shape,
                                   amdinfer::DataType::INT8);
  }
  // -construct request:

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
  if (!serverHasExtension(client, "vitis")) {
    std::cerr << "Vitis AI is not enabled. Please recompile with it enabled to "
                 "run this example\n";
    exit(0);
  }

  // Load-time parameters are used to pass one-time information to the batcher
  // and worker as it starts up. Each worker can choose to define its own
  // parameters that it pays attention to. Similarly, the batcher that the
  // worker is using may have its own parameters. Check the documentation to see
  // what may be specified.

  // +load:
  amdinfer::RequestParameters parameters;
  parameters.put("model", args.path_to_model);
  parameters.put("batch_size", args.batch_size);
  std::string endpoint = client->workerLoad("xmodel", &parameters);
  // -load:
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

  if (args.path_to_model.empty()) {
    const auto* root_str = std::getenv("AMDINFER_ROOT");
    assert(root_str != nullptr);
    fs::path root{root_str};
    args.path_to_model =
      root /
      "external/artifacts/u200_u250/resnet_v1_50_tf/resnet_v1_50_tf.xmodel";
  }

  return args;
}

int main(int argc, char* argv[]) {
  std::cout << "Running the Vitis example for ResNet50 in C++\n";

  Args args = getArgs(argc, argv);

  // +create client:
  // vitis.cpp
  const auto http_port_str = std::to_string(args.http_port);
  const auto server_addr = "http://" + args.ip + ":" + http_port_str;
  amdinfer::HttpClient client{server_addr};
  // -create client:

  // +initialize:
  std::optional<amdinfer::Server> server;
  // -initialize:
  // +start protocol:
  if (args.ip == "127.0.0.1" && !client.serverLive()) {
    std::cout << "No server detected. Starting locally...\n";
    server.emplace();
    server.value().startHttp(args.http_port);
  } else if (!client.serverLive()) {
    throw amdinfer::connection_error("Could not connect to server at " +
                                     server_addr);
  }
  // -start protocol:

  std::cout << "Waiting until the server is ready...\n";
  amdinfer::waitUntilServerReady(&client);

  std::cout << "Loading worker...\n";
  std::string endpoint = load(&client, args);
  // +wait model ready:
  amdinfer::waitUntilModelReady(&client, endpoint);
  // -wait model ready:

  // +prepare images:
  std::vector<std::string> paths = resolveImagePaths(args.path_to_image);
  Images images = preprocess(paths);
  // -prepare images:

  std::vector<amdinfer::InferenceRequest> requests =
    constructRequests(images, args.input_size);

  assert(paths.size() == requests.size());
  const auto num_requests = requests.size();

  std::cout << "Making inferences...\n";
  auto start = std::chrono::high_resolution_clock::now();
  for (auto i = 0U; i < num_requests; ++i) {
    const amdinfer::InferenceRequest& request = requests[i];
    const std::string& image_path = paths[i];

    // +validate:
    // vitis.cpp
    amdinfer::InferenceResponse response = client.modelInfer(endpoint, request);
    assert(!response.isError());

    std::vector<amdinfer::InferenceResponseOutput> outputs =
      response.getOutputs();
    // for resnet50, we expect a single output tensor
    assert(outputs.size() == 1);
    std::vector<int> top_indices = postprocess(outputs[0], args.top);
    printLabel(top_indices, args.path_to_labels, image_path);
    // -validate:
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = stop - start;
  std::cout << "Time taken for inference, postprocessing and printing: "
            << duration.count() << " ms\n";

  return 0;
}
