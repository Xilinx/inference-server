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

#include <array>    // for array
#include <cmath>    // for exp
#include <cstdint>  // for int8_t, uint64_t
#include <cstdlib>  // for getenv, size_t
#include <filesystem>
#include <fstream>  // For files
// #include <functional>             // for multiplies
#include <future>            // for future
#include <initializer_list>  // for initializer_list
#include <iostream>          // for operator<<, basic_ostream::operator<<
// #include <numeric>
#include <opencv2/core.hpp>       // for Mat, Vec3b, MatSize, Vec, CV_8SC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize
#include <queue>                  // for priority_queue, queue
#include <string>                 // for string, allocator, operator==, basi...
#include <utility>                // for pair, move
#include <vector>                 // for vector

#include "proteus/proteus.hpp"  // for InferenceResponseFuture, terminate
#include "proteus/util/pre_post/image_preprocess.hpp"
#include "proteus/util/pre_post/resnet50_postprocess.hpp"
#include "proteus/util/read_nth_line.hpp"

// // #include "migraphx/filesystem.hpp"
// #pragma GCC diagnostic push
// // Disable a warning for extra ; that appear in migraphx.hpp
// #pragma GCC diagnostic ignored "-Wpedantic"
// #include "migraphx/migraphx.hpp"  // MIGraphX C++ API
// #pragma GCC diagnostic pop

/**
 * @brief user variables: update as needed!
 * If image location is provided, the prediction is done for that image,
 * If not provided, dummy data will be created for functionality testing.
 *
 */
struct Option {
  std::filesystem::path root = std::getenv("PROTEUS_ROOT");

  std::filesystem::path model =
    root / "external/artifacts/migraphx/resnet50v2/resnet50-v2-7.onnx";
  std::filesystem::path image_location =
    root / "tests/assets/dog-3619020_640.jpg";

  // same content as tests/assets/imagenet_simple_labels.json
  std::filesystem::path class_file =
    root / "examples/python/utils/imagenet_classes.txt";

  int batch_size = 64;
  int input_size =
    224;  // the input size is set by the Resnet model (224x224 pixels)

  // values used in iterations and postprocessing
  int output_classes = 1000;
  int steps = 1;
  int topK = 5;
} options;

/** Runs an MiGraphX inference straight from the API, without going
 * through the Inference Server.  Use to verify that results for the
 * same model are the same, or to do performance checks on the GPU that bypass
 * the time overhead of the REST interface.
 *
 *
 * */

int main(int argc, char* argv[]) {
  auto* root_env = std::getenv("PROTEUS_ROOT");
  if (root_env == nullptr) {
    std::cerr << "PROTEUS_ROOT is not defined in the environment\n";
    return 1;
  }

  std::filesystem::path root(root_env);

  std::filesystem::path model =
    root / "external/artifacts/migraphx/resnet50v2/resnet50-v2-7.onnx";
  if (argc > 1) model = std::string(argv[1]);

  int batch_size(64);
  if (argc > 2) batch_size = atoi(argv[2]);
  options.batch_size = batch_size;

  // The image file path is hardcoded.  For performance testing we'll just
  // process multiple copies of the same image.
  std::filesystem::path image_filename = options.image_location;
  std::vector<std::string> paths;
  paths.push_back(image_filename.string());
  std::cout << " input image is " << image_filename << "   batch size is "
            << std::to_string(batch_size) << std::endl;

  // initialize the server
  proteus::Server server;

  auto client = proteus::NativeClient();
  auto metadata = client.serverMetadata();
  if (metadata.extensions.find("migraphx") == metadata.extensions.end()) {
    std::cout << "Migraphx support required but is not installed in this "
                 "environment.\n";
    exit(0);
  }

  //
  // load worker with required parameters
  //

  proteus::RequestParameters parameters;
  parameters.put("batch", batch_size);
  parameters.put("model", model);
  // timeout is measured in milliseconds
  parameters.put("timeout", 100);
  auto workerName = client.workerLoad("Migraphx", &parameters);
  std::cout << "loaded worker name " << workerName << std::endl;

  // Load the image and make an array of copies
  proteus::util::ImagePreprocessOptions<float, 3> options_;
  options_.normalize = true;
  options_.order = proteus::util::ImageOrder::NCHW;
  options_.mean = {0.485, 0.456, 0.406};
  options_.std = {4.367, 4.464, 4.444};
  options_.convert_color = true;
  options_.color_code = cv::COLOR_BGR2RGB;
  options_.convert_type = true;
  options_.type = CV_32FC3;
  options_.convert_scale = 1.0 / 255.0;
  auto images = proteus::util::imagePreprocess(paths, options_);

  const std::initializer_list<uint64_t> shape = {
    3, static_cast<long unsigned>(options.input_size),
    static_cast<long unsigned>(options.input_size)};
  std::queue<proteus::InferenceResponseFuture> queue;
  proteus::InferenceRequest request;
  request.addInputTensor(static_cast<void*>(images[0].data()), shape,
                         proteus::DataType::FP32);
  // Timing the prediction
  auto start = std::chrono::high_resolution_clock::now();  // Timing the start
  auto results = client.modelInfer(workerName, request);
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration =
    std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
  float time_tmp = duration.count();

  // Parse the results and report the top K results
  auto outputs = results.getOutputs();
  for (auto& output : outputs) {
    auto top_k = proteus::util::resnet50Postprocess(
      static_cast<float*>(output.getData()), output.getSize(), options.topK);
    std::cout << "Top " << options.topK << " classes:" << std::endl;
    for (int j = 0; j < options.topK; j++)
      std::cout << j + 1 << ": Index: " << top_k[j] << " :: Class: "
                << proteus::util::readNthLine(options.class_file, top_k[j])
                << std::endl;
  }

  // Running for `steps` number of times for proper benchmarking
  start = std::chrono::high_resolution_clock::now();  // Timing the start
  for (int step = 0; step < options.steps; step++) {
    for (auto i = 0; i < options.batch_size; i++) {
      proteus::InferenceRequest request;
      request.addInputTensor(static_cast<void*>(images[0].data()), shape,
                             proteus::DataType::FP32);
      auto results = client.modelInfer(workerName, request);
    }

    std::cout << " INPUT, batch size: " << request.getInputSize() << " "
              << options.batch_size << std::endl;
    // auto results = client.modelInfer(workerName, request);
  }
  // Timing the prediction
  stop = std::chrono::high_resolution_clock::now();
  duration =
    std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
  time_tmp = float(duration.count()) / options.steps;

  std::cout << "\nAverage time taken for " << options.image_location.c_str()
            << " : " << std::to_string(time_tmp) << "ms" << std::endl;

  std::cout << "Batch Size: " + std::to_string(options.batch_size) +
                 " FPS: " + std::to_string(options.batch_size / time_tmp * 1000)
            << std::endl;
  std::cout << " finished.  Time is " << std::to_string(time_tmp) << std::endl;

  return 0;
}

// clang-format off
// // This is the Migraphx C++ API main, not the inference server C+ API
// int main(int argc, char* argv[]){
// std::string model_filename =
// "/workspace/proteus/external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx";
// if(argc > 1)
//     model_filename = std::string(argv[1]);

// migraphx::onnx_options onnx_opts;
// onnx_opts.set_default_dim_value(64);
// std::cout << " parsing model file "  << model_filename << std::endl;
// migraphx::api::program program = migraphx::parse_onnx(model_filename.c_str(),
// onnx_opts); migraphx::compile_options options; options.set_offload_copy();
// program.compile(migraphx::target("gpu"), options);

// std::string image_filename =
// "/workspace/proteus/tests/assets/dog-3619020_640.jpg";
//  if(argc > 2)
//     image_filename = std::string(argv[2]);
// std::cout << " input image is " << image_filename << std::endl;

// migraphx::program_parameters pp;
// // the shape taken from the Resnet50 model should be 224x224x3.  Make the
// input image the same size. auto param_shapes =
// program.get_parameter_shapes(); for(auto&& name : param_shapes.names())
// {
//     std::cout << "adding random image to test set" << std::endl;
//     pp.add(name, migraphx::argument::generate(param_shapes[name]));
// }

// // run the inference
// auto outputs = program.eval(pp);
// auto output  = outputs[0];
// auto lens    = output.get_shape().lengths();
// auto elem_num =
//     std::accumulate(lens.begin(), lens.end(), 1,
//     std::multiplies<std::size_t>());
// float* data_ptr = reinterpret_cast<float*>(output.data());
// std::vector<float> ret(data_ptr, data_ptr + elem_num);

// We should see 1000 items returned.
// std::cout << "done.  elem is " << std::to_string(elem_num) << std::endl;
// }
// clang-format on
