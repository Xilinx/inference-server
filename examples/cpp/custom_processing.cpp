// Copyright 2021 Xilinx Inc.
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

#include <algorithm>              // for max, fill
#include <array>                  // for array
#include <cmath>                  // for exp
#include <cstdint>                // for int8_t, uint64_t, int32_t
#include <cstdlib>                // for exit, getenv, size_t
#include <initializer_list>       // for initializer_list
#include <iostream>               // for operator<<, basic_ostream::operator<<
#include <memory>                 // for allocator, allocator_traits<>::valu...
#include <opencv2/core.hpp>       // for Mat, Vec3b, MatSize, Vec, CV_8SC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize
#include <queue>                  // for priority_queue
#include <string>                 // for string, operator==, basic_string
#include <unordered_set>          // for operator==, unordered_set, unordere...
#include <utility>                // for pair
#include <vector>                 // for vector

#ifdef ENABLE_GRPC
#include <chrono>  // for seconds
#include <thread>  // for sleep_for
#endif

// +include:
#include "proteus/proteus.hpp"
// -include:

#include "proteus/util/pre_post/resnet50_postprocess.hpp"
#include "proteus/util/pre_post/resnet50_preprocess.hpp"

#ifdef ENABLE_GRPC
constexpr auto GRPC_PORT = 50051;
constexpr auto GRPC_ADDRESS = "localhost:50051";

std::string load(const std::string& path_to_xmodel) {
  // +initialize grpc:
  proteus::GrpcClient client{GRPC_ADDRESS};
  // -initialize grpc:
  while (!client.serverLive()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  auto metadata = client.serverMetadata();
  if (metadata.extensions.find("vitis") == metadata.extensions.end()) {
    std::cout << "Vitis AI support required but not found.\n";
    exit(0);
  }

  // +load grpc:
  proteus::RequestParameters parameters;
  parameters.put("model", path_to_xmodel);
  auto worker_name = client.workerLoad("Xmodel", &parameters);
  // -load grpc:

  return worker_name;
}

proteus::InferenceResponse infer(const std::string& worker_name,
                                 const proteus::InferenceRequest& request) {
  proteus::GrpcClient client{GRPC_ADDRESS};

  // +inference grpc:
  auto results = client.modelInfer(worker_name, request);
  // -inference grpc:

  return results;
}

std::vector<int> postprocess(const proteus::InferenceResponseOutput& output,
                             int k) {
  return proteus::util::resnet50Postprocess(
    static_cast<int32_t*>(output.getData()), output.getSize(), k);
}
#else
std::string load(const std::string& path_to_xmodel) {
  proteus::NativeClient client;
  auto metadata = client.serverMetadata();
  if (metadata.extensions.find("vitis") == metadata.extensions.end()) {
    std::cout << "Vitis AI support required but not found.\n";
    exit(0);
  }

  // +load native:
  proteus::RequestParameters parameters;
  parameters.put("model", path_to_xmodel);
  auto worker_name = client.workerLoad("Xmodel", &parameters);
  // -load native:

  return worker_name;
}

proteus::InferenceResponse infer(const std::string& worker_name,
                                 const proteus::InferenceRequest& request) {
  proteus::NativeClient client;
  // +inference native:
  auto response = client.modelInfer(worker_name, request);
  // -inference native:

  return response;
}

std::vector<int> postprocess(const proteus::InferenceResponseOutput& output,
                             int k) {
  return proteus::util::resnet50Postprocess(
    static_cast<int8_t*>(output.getData()), output.getSize(), k);
}
#endif

/**
 * @brief The custom processing example demonstrates how to plug in custom pre-
 * and post-processing around a call to the inference server. This example uses
 * Resnet50, a classification model, in the XModel worker. Pre- and post-
 * processing is done in C++ before and after the call to the server.
 *
 * @return int
 */
int main() {
  auto* root_env = std::getenv("PROTEUS_ROOT");
  if (root_env == nullptr) {
    std::cerr << "PROTEUS_ROOT is not defined in the environment\n";
    return 1;
  }
  std::string root = root_env;

  // +user variables: update as needed!
  const auto request_num = 4;
  const auto* path_to_xmodel =
    "${PROTEUS_ROOT}/external/artifacts/u200_u250/resnet_v1_50_tf/"
    "resnet_v1_50_tf.xmodel";
  const auto path_to_image = root + "/tests/assets/dog-3619020_640.jpg";
  // for this image, we know what we expect to receive with this XModel
  // note this is different than custom_processing.py due to how the top_k are
  // calculated. Index 230 and 154 are equal classifications
  const std::array gold_response_output = {259, 261, 260, 157, 230};
  const auto k = gold_response_output.size();
  // -user variables:

  // +initialize:
  proteus::Server server;
// -initialize:
#ifdef ENABLE_GRPC
  server.startGrpc(GRPC_PORT);
#endif

  auto worker_name = load(path_to_xmodel);

  // +prepare images:
  std::vector<std::string> paths;
  paths.reserve(request_num);
  paths.emplace_back(path_to_image);

  proteus::util::Resnet50PreprocessOptions<int8_t, 3> options;
  options.order = proteus::util::ImageOrder::NHWC;
  options.mean = {123, 107, 104};
  options.std = {1, 1, 1};
  options.normalize = true;
  auto images = proteus::util::resnet50Preprocess(paths, options);
  // -prepare images:

  // +construct request:
  const std::initializer_list<uint64_t> shape = {224, 224, 3};

  proteus::InferenceRequest request;
  request.addInputTensor(static_cast<void*>(images[0].data()), shape,
                         proteus::DataType::INT8);
  // -construct request:

  for (auto i = 0; i < request_num; i++) {
    auto results = infer(worker_name, request);

    // +validate:
    auto outputs = results.getOutputs();
    for (auto& output : outputs) {
      std::vector<int> top_k = postprocess(output, k);
      for (size_t j = 0; j < k; j++) {
        if (top_k[j] != gold_response_output.at(j)) {
          std::cerr << "Output (" << top_k[j] << ") does not match golden ("
                    << gold_response_output.at(j) << ")\n";
          return 1;
        }
      }
    }
    // -validate:
  }

  std::cout << "custom_processing.cpp: Passed\n";

  return 0;
}
