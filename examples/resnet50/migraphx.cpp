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
 * @brief This example demonstrates how you can use the MIGraphX backend to run
 * inference on an AMD GPU with a ResNet50 ONNX model. Look at the documentation
 * online for discussion around this example.
 */

#include <array>                // for array
#include <cassert>              // for assert
#include <chrono>               // for duration
#include <cstdint>              // for uint64_t
#include <cstdlib>              // for exit, getenv
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

#include "proteus/pre_post/image_preprocess.hpp"      // for ImagePrepr...
#include "proteus/pre_post/resnet50_postprocess.hpp"  // for resnet50Po...
#include "proteus/proteus.hpp"                        // for InferenceR...
#include "resnet50.hpp"                               // for Args, pars...

namespace fs = std::filesystem;

using Images = std::vector<std::vector<float>>;

/**
 * @brief Given a vector of paths to images, preprocess the images and return
 * them
 *
 * @param paths paths to images to preprocess
 * @return Images
 */
Images preprocess(const std::vector<std::string>& paths) {
  const std::array<float, 3> mean{0.485F, 0.456F, 0.406F};
  const std::array<float, 3> std{4.367F, 4.464F, 4.444F};
  const auto image_size = 224;

  // this example uses a custom image preprocessing function. You may use any
  // preprocessing logic or skip it entirely if your input data is already
  // preprocessed.
  proteus::pre_post::ImagePreprocessOptions<float, 3> options;
  options.order = proteus::pre_post::ImageOrder::NCHW;
  options.height = image_size;
  options.width = image_size;
  options.mean = mean;
  options.std = std;
  options.normalize = true;
  options.convert_color = true;
  options.color_code = cv::COLOR_BGR2RGB;
  options.convert_type = true;
  options.type = CV_32FC3;
  options.convert_scale = 1.0 / 255.0;
  return proteus::pre_post::imagePreprocess(paths, options);
}

/**
 * @brief Postprocess the output data. For ResNet50, this includes performing a
 * softmax to determine the most probable classifications.
 *
 * @param output the output from the inference server
 * @param k number of top categories to return
 * @return std::vector<int> the indices for the top k categories
 */
std::vector<int> postprocess(const proteus::InferenceResponseOutput& output,
                             int k) {
  return proteus::pre_post::resnet50Postprocess(
    static_cast<const float*>(output.getData()), output.getSize(), k);
}

/**
 * @brief Construct requests for the inference server from the input images. For
 * ResNet50, a valid request includes a single input tensor containing a square
 * image.
 *
 * @param images the input images
 * @param input_size size of the square image in pixels
 * @return std::vector<proteus::InferenceRequest>
 */
std::vector<proteus::InferenceRequest> constructRequests(const Images& images,
                                                         uint64_t input_size) {
  std::vector<proteus::InferenceRequest> requests;
  requests.reserve(images.size());

  const std::initializer_list<uint64_t> shape = {input_size, input_size, 3};

  for (const auto& image : images) {
    requests.emplace_back();
    requests.back().addInputTensor((void*)image.data(), shape,
                                   proteus::DataType::FP32);
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
std::string load(const proteus::Client* client, const Args& args) {
  // Depending on how the server is compiled, it may or may not have support for
  // a particular backend. This guard checks to make sure the server does
  // support the requested backend. If you already know it's supported, you can
  // skip this check.
  if (!serverHasExtension(client, "migraphx")) {
    std::cerr << "MIGraphX is not enabled. Please recompile with it enabled to "
              << "run this example\n";
    exit(0);
  }

  // Load-time parameters are used to pass one-time information to the batcher
  // and worker as it starts up. Each worker can choose to define its own
  // parameters that it pays attention to. Similarly, the batcher that the
  // worker is using may have its own parameters. Check the documentation to see
  // what may be specified.

  // +load
  proteus::RequestParameters parameters;
  const auto timeout_ms = 1000;  // batcher timeout value in milliseconds
  const auto batch_size = 2;

  // Required: specifies path to the model on the server for it to open
  parameters.put("model", args.path_to_model);
  // Optional: request a particular batch size to be sent to the backend. The
  // server will attempt to coalesce incoming requests into a single batch of
  // this size and pass it all to the backend.
  parameters.put("batch", batch_size);
  // Optional: specifies how long the batcher should wait for more requests
  // before sending the batch on
  parameters.put("timeout", timeout_ms);
  std::string endpoint = client->workerLoad("migraphx", &parameters);
  proteus::waitUntilModelReady(client, endpoint);
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

  if (args.path_to_model.empty()) {
    fs::path root{std::getenv("PROTEUS_ROOT")};
    args.path_to_model =
      root / "external/artifacts/onnx/resnet50v2/resnet50-v2-7.onnx";
  }

  return args;
}

int main(int argc, char* argv[]) {
  std::cout << "Running the MIGraphX example for ResNet50 in C++\n";

  Args args = getArgs(argc, argv);

  const auto http_port_str = std::to_string(args.http_port);
  proteus::HttpClient client{"http://127.0.0.1:" + http_port_str};

  std::optional<proteus::Server> server;
  if (!client.serverLive()) {
    std::cout << "No server detected. Starting locally...\n";
    server.emplace();
    server.value().startHttp(args.http_port);
  }

  std::cout << "Waiting until the server is ready...\n";
  proteus::waitUntilServerReady(&client);

  std::cout << "Loading worker...\n";
  std::string endpoint = load(&client, args);

  std::vector<std::string> paths = resolveImagePaths(args.path_to_image);
  Images images = preprocess(paths);

  std::vector<proteus::InferenceRequest> requests =
    constructRequests(images, args.input_size);

  assert(paths.size() == requests.size());
  const auto num_requests = requests.size();

  std::cout << "Making inference...\n";
  auto start = std::chrono::high_resolution_clock::now();

  // +validate:
  // migraphx.cpp
  std::vector<proteus::InferenceResponse> responses =
    proteus::inferAsyncOrdered(&client, endpoint, requests);
  assert(num_requests == responses.size());

  for (auto i = 0U; i < num_requests; ++i) {
    const proteus::InferenceResponse& response = responses[i];
    assert(!response.isError());

    std::vector<proteus::InferenceResponseOutput> outputs =
      response.getOutputs();
    // for resnet50, we expect a single output tensor
    assert(outputs.size() == 1);
    std::vector<int> top_indices = postprocess(outputs[0], args.top);
    printLabel(top_indices, args.path_to_labels, paths[i]);
  }
  // -validate:
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = stop - start;
  std::cout << "Time taken for inference, postprocessing and printing: "
            << duration.count() << " ms\n";

  return 0;
}
