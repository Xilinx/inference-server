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
 * @brief Implements the rocAL worker.
 */

#include <opencv2/opencv.hpp>     // for opencv
#include "rocal/rocal_api.h"            // for rocal
#include "rapidjson/document.h"       // for rapidjson
#include "rapidjson/istreamwrapper.h" // for rapidjson

#include <algorithm>              // for max
#include <cstddef>                // for byte, size_t
#include <cstring>                // for memcpy
#include <exception>              // for exception
#include <filesystem>             // for path
#include <fstream>                // for ifstream, operator<<
#include <map>                    // for map
#include <memory>                 // for allocator, unique_ptr
#include <ratio>                  // for micro
#include <stdexcept>              // for invalid_argument, runt...
#include <string>                 // for string, operator+, to_...
#include <thread>                 // for thread
#include <utility>                // for move
#include <vector>                 // for vector

#include "amdinfer/batching/hard.hpp"           // for BatchPtr, Batch, Batch...
#include "amdinfer/build_options.hpp"           // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/core/data_types.hpp"         // for DataType, operator<<
#include "amdinfer/core/exceptions.hpp"         // for invalid_argument, runt...
#include "amdinfer/core/inference_request.hpp"  // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"          // for ParameterMap
#include "amdinfer/declarations.hpp"             // for InferenceResponseOutput
#include "amdinfer/observation/logging.hpp"  // for AMDINFER_LOG_INFO, AMD...
#include "amdinfer/observation/metrics.hpp"  // for Metrics, MetricCounterIDs
#include "amdinfer/util/containers.hpp"      // for containerProduct
#include "amdinfer/util/memory.hpp"          // for copy
#include "amdinfer/util/queue.hpp"           // for BufferPtrsQueue
#include "amdinfer/util/string.hpp"          // for contains
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer
#include "amdinfer/workers/worker.hpp"       // for Worker, kNumBufferAuto
#include "amdinfer/buffers/vector.hpp"       // for VectorBuffer

namespace amdinfer::workers {

/**
 * @brief Rocal Operation Enum Class
 */
enum class RocalOperation {
    Resize, ResizeMirrorNormalize, CropResizeFixed, 
    CropMirrorNormalize, CropFixed, ResizeCropMirrorFixed
};

/**
 * @brief A helper function to convert string to RocalOperation enum
 */
RocalOperation getOperationType(const std::string& op) {
    if (op == "Resize") return RocalOperation::Resize;
    if (op == "ResizeMirrorNormalize") return RocalOperation::ResizeMirrorNormalize;
    if (op == "CropResizeFixed") return RocalOperation::CropResizeFixed;
    if (op == "CropMirrorNormalize") return RocalOperation::CropMirrorNormalize;
    if (op == "CropFixed") return RocalOperation::CropFixed;
    if (op == "ResizeCropMirrorFixed") return RocalOperation::ResizeCropMirrorFixed;
    std::cerr << "Unknown operation: " + op << std::endl;
    throw invalid_argument("Unknown operation: " + op);
}

/**
 * @brief The rocAL worker class
 */
class RocalWorker : public SingleThreadedWorker {
 public:
  using SingleThreadedWorker::SingleThreadedWorker;
  [[nodiscard]] std::vector<MemoryAllocators> getAllocators() const override;

 private:
  void doInit(ParameterMap* parameters) override;
  void doAcquire(ParameterMap* parameters) override;
  BatchPtr doRun(Batch* batch, const MemoryPool* pool) override;
  void doRelease() override;
  void doDestroy() override;

  void deserialize(const std::string& model_path);
  std::vector<float> parseVector(const rapidjson::Value& jsonArray);

  std::string model_path_;

  // The mode for rocalExternalSource reader
  // 0 - External File Mode
  // 1 - Raw Compressed Mode
  // 2 - Raw Uncompressed Mode
  RocalExternalSourceMode rocal_external_mode_ = ROCAL_EXTSOURCE_RAW_COMPRESSED;

  // Maximum decode width & height
  const int max_decode_width_ = 1000, max_decode_height_ = 1000; 

  // ROI values for input image
  std::vector<ROIxywh> ROI_xywh_;

  // Default channel value
  int channels_ = 3;

  // The context for an augmentation pipeline. Contains most of
  // the worker's important info such as number, data types and sizes of
  // input and output buffers
  RocalContext handle_;
};

std::vector<MemoryAllocators> RocalWorker::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

std::vector<float> RocalWorker::parseVector(const rapidjson::Value& jsonArray) {
    std::vector<float> result;
    result.reserve(jsonArray.Size());
    for (const auto& element : jsonArray.GetArray()) {
        result.push_back(element.GetFloat());
    }
    return result;
}

void RocalWorker::deserialize(const std::string& model_path) {
  namespace fs = std::filesystem;
  std::filesystem::path file_path = model_path;
  
  // check if the json model exists
  if (!std::filesystem::exists(file_path)) {
      std::cerr << "Model does not exist: " << model_path << std::endl;
      throw invalid_argument("Model does not exist: " + model_path);
  }

  // check if the model format is json
  if (file_path.extension() != ".json") {
      std::cerr << "File is not in JSON format: " << model_path << std::endl;
      throw file_read_error("File is not in JSON format: " + model_path);
  }

  std::ifstream ifs(model_path);
  if (!ifs) {
      throw file_read_error("Unable to open file: " + model_path);
  }

  rapidjson::IStreamWrapper isw(ifs);
  rapidjson::Document doc;
  doc.ParseStream(isw);

  if (doc.HasParseError()) {
      std::cerr << "JSON parsing error: " + std::to_string(doc.GetParseError()) + " at offset " + std::to_string(doc.GetErrorOffset()) << std::endl;
      throw file_read_error("JSON parsing error: " + std::to_string(doc.GetParseError()) + " at offset " + std::to_string(doc.GetErrorOffset()));
  }
  else {
      std::cout << "rocAL Model Parsing Complete" << std::endl;
  }

  [[maybe_unused]]RocalTensor input = rocalJpegExternalFileSource(handle_, RocalImageColor::ROCAL_COLOR_RGB24, false, 
      false, false, ROCAL_USE_USER_GIVEN_SIZE, max_decode_width_, max_decode_height_, RocalDecoderType::ROCAL_DECODER_TJPEG, rocal_external_mode_);
      
  if (rocalGetStatus(this->handle_) != ROCAL_OK) {
      std::string errorMessage = "JPEG source could not be initialized\n" + std::string(rocalGetErrorMessage(this->handle_)); 
      throw invalid_argument(errorMessage);
  }

  const rapidjson::Value& pipeline = doc["pipeline"];
  std::vector<RocalTensor> tensors;
  tensors.reserve(pipeline.Size());
  for (const auto& node : pipeline.GetArray()) {
      RocalOperation operationType = getOperationType(node["operation"].GetString());
      unsigned width = 0, height = 0, depth = 0;
      float x = 0, y = 0, z = 0;
      bool is_output = false;
      std::vector<float> mean, std_dev;
      bool mirror = false;
      RocalIntParam p_mirror;
      float area, aspect_ratio, x_center_drift, y_center_drift;

      RocalTensor inputTensor = (tensors.empty()) ? input : tensors.back();
      RocalTensor outputTensor;

      switch (operationType) {
          case RocalOperation::Resize:
              width = node["dest_width"].GetUint();
              height = node["dest_height"].GetUint();
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              outputTensor = rocalResize(handle_, inputTensor, width, height, is_output);
              break;
          case RocalOperation::ResizeMirrorNormalize:
              width = node["dest_width"].GetUint();
              height = node["dest_height"].GetUint();
              mean = parseVector(node["mean"]);
              std_dev = parseVector(node["std_dev"]);
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              mirror = node.HasMember("mirror") ? node["mirror"].GetBool() : false;
              p_mirror = rocalCreateIntParameter(mirror);
              outputTensor = rocalResizeMirrorNormalize(handle_, inputTensor, width, height, mean, std_dev, is_output, ROCAL_SCALING_MODE_DEFAULT, 
                                                        {}, 0, 0, ROCAL_LINEAR_INTERPOLATION, p_mirror);
              break;
          case RocalOperation::CropResizeFixed:
              width = node["dest_width"].GetUint();
              height = node["dest_height"].GetUint();
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              area = node["area"].GetFloat();
              aspect_ratio = node["aspect_ratio"].GetFloat();
              x_center_drift = node["x_center_drift"].GetFloat();
              y_center_drift = node["y_center_drift"].GetFloat();
              outputTensor = rocalCropResizeFixed(handle_, inputTensor, width, height, is_output, area, aspect_ratio, x_center_drift, y_center_drift);
              break;
          case RocalOperation::CropMirrorNormalize:
              height = node["crop_height"].GetUint();
              width = node["crop_width"].GetUint();
              x = node["start_x"].GetFloat();
              y = node["start_y"].GetFloat();
              mean = parseVector(node["mean"]);
              std_dev = parseVector(node["std_dev"]);
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              mirror = node.HasMember("mirror") ? node["mirror"].GetBool() : false;
              p_mirror = rocalCreateIntParameter(mirror);
              outputTensor = rocalCropMirrorNormalize(handle_, inputTensor, height, width, x, y, mean, std_dev, is_output, p_mirror);
              break;
          case RocalOperation::CropFixed:
              width = node["crop_width"].GetUint();
              height = node["crop_height"].GetUint();
              depth = node["crop_depth"].GetUint();
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              x = node["crop_pos_x"].GetFloat();
              y = node["crop_pos_y"].GetFloat();
              z = node["crop_pos_z"].GetFloat();
              outputTensor = rocalCropFixed(handle_, inputTensor, width, height, depth, is_output, x, y, z);
              break;
          case RocalOperation::ResizeCropMirrorFixed:
              width = node["dest_width"].GetUint();
              height = node["dest_height"].GetUint();
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              y = node["crop_h"].GetUint();
              x = node["crop_w"].GetUint();
              mirror = node.HasMember("mirror") ? node["mirror"].GetBool() : false;
              p_mirror = rocalCreateIntParameter(mirror);
              outputTensor = rocalResizeCropMirrorFixed(handle_, inputTensor, width, height, is_output, y, x, p_mirror);
              break;
          default:
              std::string errorMessage = "Unrecognized operation";
              std::cerr << errorMessage << std::endl;
              throw invalid_argument(errorMessage);
      }
      tensors.push_back(outputTensor);
  }

  if (rocalGetStatus(this->handle_) != ROCAL_OK) {
      std::string errorMessage = "Error while adding the augmentation nodes\n" + std::string(rocalGetErrorMessage(this->handle_));
      throw runtime_error(errorMessage);
  }

  // Calling the API to verify and build the augmentation graph
  if (rocalVerify(this->handle_) != ROCAL_OK) {
      std::string errorMessage = "Could not verify the rocAL pipeline\n" + std::string(rocalGetErrorMessage(this->handle_));
      throw runtime_error(errorMessage);
  }
}

void RocalWorker::doInit(ParameterMap* parameters) {
  // default batch size; client may request a change. Arbitrarily set to 64
  const int default_batch_size = 64;

  this->batch_size_ = default_batch_size;
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif
  // stringstream used for formatting logger messages
  std::string msg;
  std::stringstream smsg(msg);
  AMDINFER_LOG_INFO(logger, " RocalWorker::doInit \n");

  if (parameters->has("batch")) {
    this->batch_size_ = parameters->get<int>("batch");
    this->ROI_xywh_.resize(this->batch_size_);
  }
  if (parameters->has("model")) {
    this->model_path_ = parameters->get<std::string>("model");
  }
  else {
    AMDINFER_LOG_ERROR(
      logger, "RocalWorker parameters required:  \"model\": \"<filepath>\"");
    // Throwing an exception causes server to delete this worker instance.
    // Client must try again.
    throw invalid_argument(
      "model file argument missing from model load request");
  }
  
  this->handle_ = rocalCreate(batch_size_, RocalProcessMode::ROCAL_PROCESS_CPU, 0, 1);
  if (rocalGetStatus(this->handle_) != ROCAL_OK) {
    AMDINFER_LOG_ERROR(
      logger, "RocalWorker handle creation failed");
    // Throwing an exception causes server to delete this worker instance.
    // Client must try again.
    throw runtime_error(
      "could not create rocAL context");
  }
  deserialize(this->model_path_);
}

void RocalWorker::doAcquire(ParameterMap* parameters) { (void)parameters; }

BatchPtr RocalWorker::doRun(Batch* batch, [[maybe_unused]]const MemoryPool* pool) {
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  if (rocalGetStatus(this->handle_) != ROCAL_OK) {
      std::cerr << "Rocal graph not properly intialized" << std::endl;
      return nullptr;
  }
  util::Timer timer;
  timer.add("rocal_batch_start");

  std::vector<unsigned char *> input_batch_buffer; 

  for (unsigned int j = 0; j < batch->size(); j++) {
      const auto& req = batch->getRequest(j);
      const auto& inputs = req->getInputs();
  
      if (inputs.size() != 1) {
          req->runCallbackError("Only one input tensor should be present");
          continue;
      }
      const auto& input = inputs.at(0);
      const auto& input_shape = input.getShape();

      assert(input_shape.size() == 1); // compressed raw buffer should have shape length = 1
      ROI_xywh_[j].h = input_shape[0];
      auto input_data = static_cast<unsigned char*>(input.getData());
      input_batch_buffer.push_back(input_data);
  }

  bool eos = true; // eos value is always true since there will be only one input tensor 
  rocalExternalSourceFeedInput(handle_, {}, false, input_batch_buffer, ROI_xywh_, max_decode_width_, max_decode_height_, channels_, RocalExternalSourceMode(rocal_external_mode_), 
                                RocalTensorLayout(0), eos);

  AMDINFER_LOG_INFO(logger, "Beginning rocalRun eval");

  timer.add("eval_start");
  if (rocalRun(this->handle_) != ROCAL_OK) {
      AMDINFER_LOG_ERROR(logger, "rocalRun failed to run");
      throw runtime_error("rocalRun failed to run");
  }
  timer.add("eval_end");

  auto eval_duration_us = timer.count<std::micro>("eval_start", "eval_end");
  [[maybe_unused]] auto eval_duration_s = eval_duration_us / std::mega::num;
  AMDINFER_LOG_INFO(
    logger,
    std::string("Finished rocalRun eval; batch size: ") +
      std::to_string(batch_size_) +
      "  elapsed time: " + std::to_string(eval_duration_us) +
      " us.  Images/sec: " + std::to_string(batch_size_ / (eval_duration_s)));


  BatchPtr new_batch = batch->propagate();
  
  RocalTensorList output_tensor_list = rocalGetOutputTensors(handle_);
  assert(output_tensor_list->size() == batch->size());

  // RocalTensor output_tensor = output_tensor_list->at(0);

  // unsigned char *out_buffer = nullptr;
  // float * out_buffer_f = static_cast<float *>output_tensor->buffer();
  size_t max_decoded_size = 0;
  for (unsigned int j = 0; j < batch->size(); j++) {
      const auto & output_tensor = output_tensor_list->at(j);
      // const int n = output_tensor->dims().at(0);
      const int h = output_tensor->dims().at(1);
      const int w = output_tensor->dims().at(2);
      const int c = output_tensor->dims().at(3);
      // std::cout << n << " " << c << " " << height << " " << width << " " << std::endl;
      size_t decoded_size = output_tensor->data_size();
      if (decoded_size > max_decoded_size) {
          max_decoded_size = decoded_size;
      }

      const auto& req = batch->getRequest(j);
      auto new_request = req->propagate();
      std::vector<int64_t> shape{h, w, c};

      new_request->addInputTensor(nullptr, shape, amdinfer::DataType::Uint8,
                                "output");
      new_batch->addRequest(new_request);
  }

  std::vector<amdinfer::BufferPtr> input_buffers;
  input_buffers.emplace_back(std::make_unique<amdinfer::VectorBuffer>(
    max_decoded_size * batch_size_));

  for (unsigned int k = 0; k < batch->size(); k++) {
      auto* data_ptr = input_buffers.at(0)->data(k * max_decoded_size);
      const auto& req = new_batch->getRequest(k);
      req->setInputTensorData(k, data_ptr);
      const auto& output_tensor = output_tensor_list->at(k);
      const auto& data = output_tensor->buffer();
      const auto decoded_size = output_tensor->data_size();
      amdinfer::util::copy(data, static_cast<std::byte*>(data_ptr),
                          decoded_size);
      
      new_batch->setModel(k, "RocalModel");
  }

  new_batch->setBuffers(std::move(input_buffers), {});
  
  timer.add("rocal_batch_end");
  auto duration = timer.count<std::micro>("rocal_batch_start", "rocal_batch_end");
  AMDINFER_LOG_INFO(logger, "rocAL preprocessing completed in " + std::to_string(duration) + " ms");

  return new_batch;
}

void RocalWorker::doRelease() { rocalRelease(this->handle_); }
void RocalWorker::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::RocalWorker("RocAL", "CPU", true);
}
}  // extern C
