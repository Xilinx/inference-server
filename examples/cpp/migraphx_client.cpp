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

// // #include "migraphx/filesystem.hpp"
// #pragma GCC diagnostic push
// // Disable a warning for extra ; that appear in migraphx.hpp
// #pragma GCC diagnostic ignored "-Wpedantic"
// #include "migraphx/migraphx.hpp"  // MIGraphX C++ API
// #pragma GCC diagnostic pop
/**
 * @brief This method will read the class file and returns class name
 *
 * @param filename: The location of the file containing class names
 * @param N: Nth value from the class file
 * @return std::string: Name of the class
 */
std::string ReadNthLine(const std::string filename, int N) {
  std::ifstream in(filename);
  std::string line;
  line.reserve(100);  // for performance
  // skip N lines
  for (int i = 0; i < N; ++i) std::getline(in, line);

  std::getline(in, line);
  return line;
}

/**
 * @brief Image preprocessing helper.  Crops the input `img` from center to a specific dimension specified
 *
 * @param img: Image which needs to be cropped
 * @param height: height of the output image
 * @param width: width of the output image
 * @return cv::Mat: Image cropped to required shape
 */
cv::Mat center_crop(cv::Mat img, int height, int width) {
  const int offsetW = (img.cols - width) / 2;
  const int offsetH = (img.rows - height) / 2;
  const cv::Rect roi(offsetW, offsetH, width, height);
  img = img(roi);
  return img;
}


/**
 * @brief Utility function:  fetches an image or images from file and returns an
 * array that can be passed as data to the migraphx worker.
 *
 * @param paths
 * @param img_size
 * @param method
 * @return std::vector<std::vector<float>>
 */
std::vector<std::vector<float>> preprocess(
  std::filesystem::path const& image_filename, size_t batch_size, size_t image_size) {


  // placeholder
  std::vector<std::vector<float>> outputs;
  outputs.reserve(batch_size);

  // load the image
  auto img = cv::imread(image_filename.c_str());
  if(img.empty()) {
    std::cout << "Unable to load image " << image_filename.c_str() << std::endl;
    exit(1);
  }
  //
  // Preprocess
  //
  std::vector<float> output1;

  cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
  auto new_size = cv::Size(256, 256);
  cv::resize(img, img, new_size, cv::INTER_LINEAR);
  img = center_crop(img, image_size, image_size);
  img.convertTo(img, CV_32FC3, 1.0 / 255.0);
  img = img.isContinuous() ? img : img.clone();

  const std::array mean = {0.485, 0.456, 0.406};
  const std::array std = {0.229, 0.224, 0.225};

  // Copy to the vector, normalizing each pixel
  auto size = img.size[0] * img.size[1] * 3;
  output1.resize(size);
  for (size_t c = 0; c < 3; c++) {
    for (size_t h = 0; h < image_size; h++) {
      for (size_t w = 0; w < image_size; w++) {
        output1[(c * image_size * image_size) + (h * image_size) + w] =
          (img.at<cv::Vec3f>(h, w)[c] - mean.at(c)) / std.at(c);
      }
    }
  }
  for (size_t index = 0; index < batch_size; index++) {
    outputs.emplace_back(output1);
    // auto& output = outputs[index++];
  } 
  std::cout << "preprocess() returning vector of " << std::to_string(outputs.size())<< " images of " << outputs[batch_size-1].size() << std::endl;

  return outputs;
}


/**
 * @brief Calculate softmax of the data.
 *         This is identical to processing in pt_zendnn_client.cpp
 *
 * @param data pointer to the raw data
 * @param size number of elements in the raw data
 * @param result pointer to store the computed results
 */
void calc_softmax(const float* data, size_t size, double* result) {
  double sum = 0;

  auto max = data[0];
  for (size_t i = 1; i < size; i++) {
    if (data[i] > max) {
      max = data[i];
    }
  }

  for (size_t i = 0; i < size; i++) {
    result[i] = exp(data[i] - max);
    sum += result[i];
  }

  for (size_t i = 0; i < size; i++) {
    result[i] /= sum;
  }
}

/**
 * @brief After running softmax, get the labels associated with the top k values.
 *         This is identical to processing in pt_zendnn_client.cpp
 *
 * @param d pointer to the data
 * @param size number of elements in the data
 * @param k number of top elements to return
 * @return std::vector<int>
 */
std::vector<int> get_top_k(const double* d, int size, int k) {
  std::priority_queue<std::pair<float, int>> q;

  for (auto i = 0; i < size; ++i) {
    q.push(std::pair<float, int>(d[i], i));
  }
  std::vector<int> topKIndex;
  for (auto i = 0; i < k; ++i) {
    std::pair<float, int> ki = q.top();
    q.pop();
    topKIndex.push_back(ki.second);
  }
  return topKIndex;
}

/**
 * @brief Perform postprocessing of the data. This is identical to processing in pt_zendnn_client.cpp
 *
 * @param output output from Proteus
 * @param k number of top elements to return
 * @return std::vector<int>
 */
std::vector<int> postprocess(proteus::InferenceResponseOutput& output, int k) {
  auto* data = static_cast<std::vector<float>*>(output.getData());
  auto size = output.getSize();

  std::vector<double> softmax;
  softmax.resize(size);

  calc_softmax(data->data(), size, softmax.data());
  return get_top_k(softmax.data(), size, k);
}


/**
 * @brief user variables: update as needed!
 * If image location is provided, the prediction is done for that image,
 * If not provided, dummy data will be created for functionality testing.
 *
 */
struct Option {
  std::filesystem::path root = std::getenv("PROTEUS_ROOT");

  std::filesystem::path model =
    root / "external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx";
  std::filesystem::path image_location =
    root / "tests/assets/dog-3619020_640.jpg";

  // same content as tests/assets/imagenet_simple_labels.json
  std::filesystem::path class_file = root /
    "/examples/python/utils/imagenet_classes.txt";

  int batch_size = 640;
  int input_size = 224;  // the input size is set by the Resnet model (224x224 pixels)
  // int output_classes = 1000;

  // values used in iterations and postprocessing
  int output_classes = 1000;
  int warmup_step = 5;
  int steps = 10;
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
    root / "external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx";
  if (argc > 1) model = std::string(argv[1]);

  int batch_size(64);
  if(argc > 2) batch_size = atoi(argv[2]);
  options.batch_size = batch_size;

    // The image file path is hardcoded.  For performance testing we'll just 
    // process multiple copies of the same image.
  std::filesystem::path image_filename = options.image_location;    
    std::cout << " input image is " << image_filename << "   batch size is " << std::to_string(batch_size) << std::endl;  

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
  auto images = preprocess(image_filename, batch_size, 224);

  const std::initializer_list<uint64_t> shape = {
    3, static_cast<long unsigned>(options.input_size),
    static_cast<long unsigned>(options.input_size)
  };
  std::queue<proteus::InferenceResponseFuture> queue;
  proteus::InferenceRequest request;
  request.addInputTensor(static_cast<void*>(images[0].data()), shape,
                          proteus::DataType::FP32);
  // Timing the prediction
  auto start = std::chrono::high_resolution_clock::now();  // Timing the start 
  auto results = client.modelInfer(workerName, request); 
  auto stop = std::chrono::high_resolution_clock::now(); 
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
  float time_tmp = duration.count();

  // Parse the results and report the top K results
  auto outputs = results.getOutputs();
  for (auto& output : outputs) {
    auto top_k = postprocess(output, options.topK);
    std::cout << "Top " << options.topK << " classes:" << std::endl;
    for (int j = 0; j < options.topK; j++)
      std::cout << j + 1 << ": Index: " << top_k[j]
                << " :: Class: " << ReadNthLine(options.class_file,
                top_k[j])
                << std::endl;
  }

  // Running for `steps` number of times for proper benchmarking
  start = std::chrono::high_resolution_clock::now();  // Timing the start 
  for (int step = 0; step < options.steps; step++) {
      for (auto i = 0; i < options.batch_size; i++) {
        proteus::InferenceRequest request;
        request.addInputTensor(static_cast<void*>(images[i].data()), shape,
                               proteus::DataType::FP32);
        auto results = client.modelInfer(workerName, request);
      }

    std::cout <<" INPUT, batch size: " << request.getInputSize() << " " << options.batch_size << std::endl;
    // auto results = client.modelInfer(workerName, request);
  }
  // Timing the prediction
  stop = std::chrono::high_resolution_clock::now();
  duration =
    std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
  time_tmp = float(duration.count()) / options.steps;
    
  std::cout << "\nAverage time taken for " << options.image_location.c_str() << " : " <<
                  std::to_string(time_tmp) << "ms" << std::endl;

  std::cout << "Batch Size: " + std::to_string(options.batch_size) +
                  " FPS: " +
                  std::to_string(options.batch_size / time_tmp * 1000)
            << std::endl;
  std::cout << " finished.  Time is " << std::to_string(time_tmp) << std::endl;

  return 0;
}

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
