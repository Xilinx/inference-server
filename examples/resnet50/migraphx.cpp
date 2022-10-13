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
#include <ratio>                // for milli
#include <string>               // for string
#include <vector>               // for vector

#include "proteus/client_operators/infer_async.hpp"        // for inferAsync...
#include "proteus/proteus.hpp"                             // for InferenceR...
#include "proteus/util/pre_post/image_preprocess.hpp"      // for ImagePrepr...
#include "proteus/util/pre_post/resnet50_postprocess.hpp"  // for resnet50Po...
#include "resnet50.hpp"                                    // for Args, pars...

namespace fs = std::filesystem;

using Images = std::vector<std::vector<float>>;

Images preprocess(const std::vector<std::string>& paths) {
  const std::array<float, 3> mean{0.485F, 0.456F, 0.406F};
  const std::array<float, 3> std{4.367F, 4.464F, 4.444F};

  proteus::util::ImagePreprocessOptions<float, 3> options;
  options.order = proteus::util::ImageOrder::NCHW;
  options.mean = mean;
  options.std = std;
  options.normalize = true;
  options.convert_color = true;
  options.color_code = cv::COLOR_BGR2RGB;
  options.convert_type = true;
  options.type = CV_32FC3;
  options.convert_scale = 1.0 / 255.0;
  return proteus::util::imagePreprocess(paths, options);
}

std::vector<int> postprocess(const proteus::InferenceResponseOutput& output,
                             int k) {
  return proteus::util::resnet50Postprocess(
    static_cast<const float*>(output.getData()), output.getSize(), k);
}

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

std::string load(proteus::Client* client, const Args& args) {
  if (!serverHasExtension(client, "migraphx")) {
    std::cerr << "MIGraphX is not enabled. Please recompile with it enabled to "
              << "run this example\n";
    exit(1);
  }

  // +load
  proteus::RequestParameters parameters;
  // batcher timeout value in milliseconds
  const auto timeout_ms = 1000;
  parameters.put("model", args.path_to_model);
  parameters.put("timeout", timeout_ms);
  std::string endpoint = client->workerLoad("migraphx", &parameters);
  // -load
  return endpoint;
}

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
  Args args = getArgs(argc, argv);

  proteus::Server server;
#ifdef PROTEUS_ENABLE_REST
  server.startHttp(args.http_port);
#else
  std::cerr << "HTTP/REST is not enabled. Please recompile the library with it "
            << "enabled to run this example.\n";
  exit(1);
#endif

  const auto http_port_str = std::to_string(args.http_port);
  proteus::HttpClient client{"http://127.0.0.1:" + http_port_str};

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
