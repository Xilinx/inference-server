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
 * @brief This example demonstrates how you can use the PT+ZenDNN backend to run
 * inference on an AMD CPU with a ResNet50 PyTorch model. Look at the
 * documentation online for discussion around this example.
 */

#include <algorithm>            // for copy, max
#include <array>                // for array
#include <cassert>              // for assert
#include <chrono>               // for duration
#include <cstdint>              // for uint64_t
#include <cstdlib>              // for exit, getenv
#include <exception>            // for exception
#include <filesystem>           // for path, oper...
#include <initializer_list>     // for initialize...
#include <iostream>             // for operator<<
#include <memory>               // for allocator
#include <opencv2/core.hpp>     // for CV_32FC3
#include <opencv2/imgproc.hpp>  // for COLOR_BGR2RGB
#include <ratio>                // for milli
#include <string>               // for string
#include <vector>               // for vector

#include "amdinfer/amdinfer.hpp"                       // for InferenceR...
#include "amdinfer/pre_post/image_preprocess.hpp"      // for ImagePrepr...
#include "amdinfer/pre_post/resnet50_postprocess.hpp"  // for resnet50Po...
#include "resnet50.hpp"                                // for Args, pars...

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
  const auto convert_scale = 1 / 255.0;

  // this example uses a custom image preprocessing function. You may use any
  // preprocessing logic or skip it entirely if your input data is already
  // preprocessed.
  amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
  options.order = amdinfer::pre_post::ImageOrder::NCHW;
  options.mean = mean;
  options.std = std;
  options.normalize = true;
  options.convert_color = true;
  options.color_code = cv::COLOR_BGR2RGB;
  options.convert_type = true;
  options.type = CV_32FC3;
  options.convert_scale = convert_scale;
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
    static_cast<const float*>(output.getData()), output.getSize(), k);
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
  std::vector<amdinfer::InferenceRequest> requests;
  requests.reserve(images.size());

  const std::initializer_list<uint64_t> shape = {input_size, input_size, 3};

  for (const auto& image : images) {
    requests.emplace_back();
    // NOLINTNEXTLINE(google-readability-casting)
    requests.back().addInputTensor((void*)image.data(), shape,
                                   amdinfer::DataType::Fp32);
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
  if (!serverHasExtension(client, "ptzendnn")) {
    std::cerr
      << "PT+ZenDNN is not enabled. Please recompile with it enabled to "
      << "run this example\n";
    exit(0);
  }

  // Load-time parameters are used to pass one-time information to the batcher
  // and worker as it starts up. Each worker can choose to define its own
  // parameters that it pays attention to. Similarly, the batcher that the
  // worker is using may have its own parameters. Check the documentation to see
  // what may be specified.

  amdinfer::ParameterMap parameters;
  parameters.put("model", args.path_to_model);
  parameters.put("input_size", args.input_size);
  parameters.put("output_classes", args.output_classes);
  parameters.put("batch_size", args.batch_size);
  return client->workerLoad("ptzendnn", &parameters);
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
      root / "external/artifacts/pytorch/resnet50_pretrained.pt";
  }

  return args;
}

int main(int argc, char* argv[]) {
  try {
    std::cout << "Running the PT+ZenDNN example for ResNet50 in C++\n";

    Args args = getArgs(argc, argv);

    // +create client
    // ptzendnn.cpp
    amdinfer::NativeClient client;

    std::cout << "Starting server locally...\n";

    amdinfer::Server server;

    std::cout << "Waiting until the server is ready...\n";
    amdinfer::waitUntilServerReady(&client);
    // -create client

    if (!args.endpoint.empty()) {
      std::cout << "Ignoring the --endpoint flag when using the NativeClient\n";
    }
    std::cout << "Loading worker...\n";
    std::string endpoint = load(&client, args);

    std::vector<std::string> paths = resolveImagePaths(args.path_to_image);
    Images images = preprocess(paths);

    std::vector<amdinfer::InferenceRequest> requests =
      constructRequests(images, args.input_size);

    assert(paths.size() == requests.size());
    const auto num_requests = requests.size();

    std::cout << "Making inference...\n";
    auto start = std::chrono::high_resolution_clock::now();
    for (auto i = 0U; i < num_requests; ++i) {
      const amdinfer::InferenceRequest& request = requests[i];
      const std::string& image_path = paths[i];

      // +validate
      // ptzendnn.cpp
      amdinfer::InferenceResponseFuture future =
        client.modelInferAsync(endpoint, request);
      amdinfer::InferenceResponse response = future.get();
      assert(!response.isError());

      std::vector<amdinfer::InferenceResponseOutput> outputs =
        response.getOutputs();
      // for resnet50, we expect a single output tensor
      assert(outputs.size() == 1);
      std::vector<int> top_indices = postprocess(outputs[0], args.top);
      printLabel(top_indices, args.path_to_labels, image_path);
      // -validate
    }
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
