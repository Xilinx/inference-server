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
#include <ratio>             // for milli
#include <string>            // for string
#include <vector>            // for vector

// +include:
#include "proteus/proteus.hpp"
// -include:

#include "proteus/util/pre_post/image_preprocess.hpp"
#include "proteus/util/pre_post/resnet50_postprocess.hpp"
#include "resnet50.hpp"

namespace fs = std::filesystem;

using Images = std::vector<std::vector<int8_t>>;

Images preprocess(const std::vector<std::string>& paths) {
  const std::array<int8_t, 3> mean{123, 107, 104};
  const std::array<int8_t, 3> std{1, 1, 1};

  proteus::util::ImagePreprocessOptions<int8_t, 3> options;
  options.order = proteus::util::ImageOrder::NHWC;
  options.mean = mean;
  options.std = std;
  options.normalize = true;
  return proteus::util::imagePreprocess(paths, options);
}

std::vector<int> postprocess(const proteus::InferenceResponseOutput& output,
                             int k) {
  return proteus::util::resnet50Postprocess(
    static_cast<const int8_t*>(output.getData()), output.getSize(), k);
}

std::vector<proteus::InferenceRequest> constructRequests(const Images& images,
                                                         uint64_t input_size) {
  // +construct request:
  std::vector<proteus::InferenceRequest> requests;
  requests.reserve(images.size());

  const std::initializer_list<uint64_t> shape = {input_size, input_size, 3};

  for (const auto& image : images) {
    requests.emplace_back();
    requests.back().addInputTensor((void*)image.data(), shape,
                                   proteus::DataType::INT8);
  }
  // -construct request:

  return requests;
}

std::string load(const proteus::Client* client, const Args& args) {
  if (!serverHasExtension(client, "vitis")) {
    std::cerr << "Vitis AI is not enabled. Please recompile with it enabled to "
                 "run this example\n";
    exit(1);
  }

  // +load:
  proteus::RequestParameters parameters;
  parameters.put("model", args.path_to_model);
  std::string endpoint = client->workerLoad("xmodel", &parameters);
  // -load:
  return endpoint;
}

Args getArgs(int argc, char** argv) {
  Args args = parseArgs(argc, argv);

  if (args.path_to_model.empty()) {
    fs::path root{std::getenv("PROTEUS_ROOT")};
    args.path_to_model =
      root /
      "external/artifacts/u200_u250/resnet_v1_50_tf/resnet_v1_50_tf.xmodel";
  }

  return args;
}

/**
 * @brief The custom processing example demonstrates how to plug in custom pre-
 * and post-processing around a call to the inference server. This example uses
 * Resnet50, a classification model, in the XModel worker. Pre- and post-
 * processing is done in C++ before and after the call to the server.
 *
 * @return int
 */
int main(int argc, char* argv[]) {
  Args args = getArgs(argc, argv);

  // +initialize:
  proteus::Server server;
  // -initialize:
#ifdef PROTEUS_ENABLE_REST
  // +start protocol:
  server.startHttp(args.http_port);
  // -start protocol:
#else
  std::cerr << "HTTP/REST is not enabled. Please recompile with it enabled to "
            << "run this example.\n";
  exit(1);
#endif

  // +create client:
  // vitis.cpp
  const auto http_port_str = std::to_string(args.http_port);
  proteus::HttpClient client{"http://127.0.0.1:" + http_port_str};

  std::cout << "Waiting until the server is ready...\n";
  proteus::waitUntilServerReady(&client);
  // -create client:

  std::cout << "Loading worker...\n";
  std::string endpoint = load(&client, args);
  // +wait model ready:
  proteus::waitUntilModelReady(&client, endpoint);
  // -wait model ready:

  // +prepare images:
  std::vector<std::string> paths = resolveImagePaths(args.path_to_image);
  Images images = preprocess(paths);
  // -prepare images:

  std::vector<proteus::InferenceRequest> requests =
    constructRequests(images, args.input_size);

  assert(paths.size() == requests.size());
  const auto num_requests = requests.size();

  std::cout << "Making inference...\n";
  auto start = std::chrono::high_resolution_clock::now();
  for (auto i = 0U; i < num_requests; ++i) {
    const proteus::InferenceRequest& request = requests[i];
    const std::string& image_path = paths[i];

    // +validate:
    // vitis.cpp
    proteus::InferenceResponse response = client.modelInfer(endpoint, request);
    assert(!response.isError());

    std::vector<proteus::InferenceResponseOutput> outputs =
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
