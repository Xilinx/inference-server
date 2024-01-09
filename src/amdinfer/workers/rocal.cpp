// Copyright 2022-2023 Advanced Micro Devices, Inc.
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

#include <rocal_api.h>            // for rocal
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

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
    throw std::invalid_argument("Unknown operation: " + op);
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

  // the folder path to be loaded.
  std::filesystem::path folder_path_;

  std::string model_path_;
  
  // The context for an augmentation pipeline. Contains most of
  // the worker's important info such as number, data types and sizes of
  // input and output buffers
  RocalContext handle_;

};

std::vector<MemoryAllocators> RocalWorker::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

std::vector<float> parseVector(const rapidjson::Value& jsonArray) {
    std::vector<float> result;
    for (const auto& element : jsonArray.GetArray()) {
        result.push_back(element.GetFloat());
    }
    return result;
}

void RocalWorker::deserialize(const std::string& model_path) {
  std::cout << "Deserializing start" << std::endl;
  std::ifstream ifs(model_path);
  rapidjson::IStreamWrapper isw(ifs);
  rapidjson::Document doc;
  std::cout << "parse stream start" << std::endl;
  doc.ParseStream(isw);
  std::cout << "parse stream end" << std::endl;
  if (!doc.IsObject()) {
      throw std::invalid_argument("Unable to parse json file");
  }
  std::cout << "Deserializing jpeg start" << std::endl;
  const int shard_count = 0;
  const int width = 224, height = 224;

  // to be replaced with external file source
  RocalImage input = rocalJpegFileSource(handle_, folder_path_.c_str(), RocalImageColor::ROCAL_COLOR_RGB24, shard_count, false, false, false, ROCAL_USE_USER_GIVEN_SIZE, width, height);
  if (rocalGetStatus(this->handle_) != ROCAL_OK) {
      std::string errorMessage = "JPEG source could not be initialized\n" + std::string(rocalGetErrorMessage(this->handle_)); 
      throw std::invalid_argument(errorMessage);
  }
std::cout << "Deserializing jpeg end" << folder_path_.c_str() << std::endl;
  const rapidjson::Value& pipeline = doc["pipeline"];
  std::vector<RocalImage> images;

  for (const auto& node : pipeline.GetArray()) {
      RocalOperation operationType = getOperationType(node["operation"].GetString());
      
      unsigned width = 0, height = 0, depth = 0;
      float x = 0, y = 0, z = 0;
      bool is_output = false;
      std::vector<float> mean, std_dev;
      bool mirror = false;
      RocalIntParam p_mirror;
      float area, aspect_ratio, x_center_drift, y_center_drift;

      RocalImage inputImage = (images.empty()) ? input : images.back();
      RocalImage outputImage;
      
      switch (operationType) {
          case RocalOperation::Resize:
              width = node["dest_width"].GetUint();
              height = node["dest_height"].GetUint();
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              outputImage = rocalResize(handle_, inputImage, width, height, is_output);
              break;
          case RocalOperation::ResizeMirrorNormalize:
              width = node["dest_width"].GetUint();
              height = node["dest_height"].GetUint();
              mean = parseVector(node["mean"]);
              std_dev = parseVector(node["std_dev"]);
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              mirror = node.HasMember("mirror") ? node["mirror"].GetBool() : false;
              p_mirror = rocalCreateIntParameter(mirror);
              outputImage = rocalResizeMirrorNormalize(handle_, inputImage, width, height, mean, std_dev, is_output, p_mirror);
              break;
          case RocalOperation::CropResizeFixed:
              width = node["dest_width"].GetUint();
              height = node["dest_height"].GetUint();
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              area = node["area"].GetFloat();
              aspect_ratio = node["aspect_ratio"].GetFloat();
              x_center_drift = node["x_center_drift"].GetFloat();
              y_center_drift = node["y_center_drift"].GetFloat();
              outputImage = rocalCropResizeFixed(handle_, inputImage, width, height, is_output, area, aspect_ratio, x_center_drift, y_center_drift);
              break;
          case RocalOperation::CropMirrorNormalize:
              depth = node["crop_depth"].GetUint();
              height = node["crop_height"].GetUint();
              width = node["crop_width"].GetUint();
              x = node["start_x"].GetFloat();
              y = node["start_y"].GetFloat();
              z = node["start_z"].GetFloat();
              mean = parseVector(node["mean"]);
              std_dev = parseVector(node["std_dev"]);
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              mirror = node.HasMember("mirror") ? node["mirror"].GetBool() : false;
              p_mirror = rocalCreateIntParameter(mirror);
              outputImage = rocalCropMirrorNormalize(handle_, inputImage, depth, height, width, x, y, z, mean, std_dev, is_output, p_mirror);
              break;
          case RocalOperation::CropFixed:
              width = node["crop_width"].GetUint();
              height = node["crop_height"].GetUint();
              depth = node["crop_depth"].GetUint();
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              x = node["crop_pos_x"].GetFloat();
              y = node["crop_pos_y"].GetFloat();
              z = node["crop_pos_z"].GetFloat();
              outputImage = rocalCropFixed(handle_, inputImage, width, height, depth, is_output, x, y, z);
              break;
          case RocalOperation::ResizeCropMirrorFixed:
              width = node["dest_width"].GetUint();
              height = node["dest_height"].GetUint();
              is_output = node.HasMember("is_output") ? node["is_output"].GetBool() : false;
              y = node["crop_h"].GetUint();
              x = node["crop_w"].GetUint();
              mirror = node.HasMember("mirror") ? node["mirror"].GetBool() : false;
              p_mirror = rocalCreateIntParameter(mirror);
              outputImage = rocalResizeCropMirrorFixed(handle_, inputImage, width, height, is_output, y, x, p_mirror);
              break;
          default:
              std::string errorMessage = "Unrecognized operation";
              throw std::invalid_argument(errorMessage);
      }
      images.push_back(outputImage);
  }

  if (rocalGetStatus(handle_) != ROCAL_OK) {
      std::string errorMessage = "Error while adding the augmentation nodes\n" + std::string(rocalGetErrorMessage(this->handle_));
      throw std::invalid_argument(errorMessage);
  }

  // Calling the API to verify and build the augmentation graph
  if (rocalVerify(handle_) != ROCAL_OK) {
      std::string errorMessage = "Could not verify the rocAL pipeline\n" + std::string(rocalGetErrorMessage(this->handle_));
      throw std::invalid_argument(errorMessage);
  }
}

void RocalWorker::doInit(ParameterMap* parameters) {
  // default batch size; client may request a change. Arbitrarily set to 64
  const int default_batch_size = 64;

  batch_size_ = default_batch_size;
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif
  // stringstream used for formatting logger messages
  std::string msg;
  std::stringstream smsg(msg);
  std::cout << "doINit" << std::endl;
  AMDINFER_LOG_INFO(logger, " RocalWorker::doInit \n");

  if (parameters->has("batch")) {
    this->batch_size_ = parameters->get<int>("batch");
  }
  if (parameters->has("folder")) {
    this->folder_path_ = parameters->get<std::string>("folder");
  }
  if (parameters->has("model")) {
    this->model_path_ = parameters->get<std::string>("model");
  }
  else {
    AMDINFER_LOG_ERROR(
      logger, "RocalWorker parameters required:  \"model\": \"<filepath>\"");
    // Throwing an exception causes server to delete this worker instance.
    // Client must try again.
    throw std::invalid_argument(
      "model file argument missing from model load request");
  }
  std::cout << "model path: " << this->model_path_ << std::endl;
  this->handle_ = rocalCreate(batch_size_, RocalProcessMode::ROCAL_PROCESS_CPU, 0, 1);
  std::cout << "rocal created" << std::endl;
  if (rocalGetStatus(this->handle_) != ROCAL_OK) {
    AMDINFER_LOG_ERROR(
      logger, "RocalWorker handle creation failed");
    // Throwing an exception causes server to delete this worker instance.
    // Client must try again.
    throw std::invalid_argument(
      "could not create rocAL context");
  }
  std::cout << "Deserializing" << std::endl;
  deserialize(this->model_path_);
  std::cout << "Deserializing done" << std::endl;
  // Calling the API to verify and build the augmentation graph
  if (rocalVerify(handle_) != ROCAL_OK) {
    std::cout << "Could not verify the rocAL graph" << std::endl;
  }
}

void RocalWorker::doAcquire(ParameterMap* parameters) { (void)parameters; }

BatchPtr RocalWorker::doRun(Batch* batch, [[maybe_unused]]const MemoryPool* pool) {
// #ifdef AMDINFER_ENABLE_LOGGING
//   const auto& logger = this->getLogger();
// #endif

  // stringstream used for formatting logger messages
  std::string msg;
  std::stringstream smsg(msg);

  //
  // Wait for requests from the batcher in an infinite loop.  This thread will
  // run, waiting for more input, until the server kills it.  If a bad request
  // causes an exception, the server will return a REST failure message to the
  // client and continue waiting for requests.
  //

  util::Timer timer;
  timer.add("batch_start");

  // We only need to look at the 0'th request to set up evaluation, because
  // its input pointers (one for each input) are the base addresses of the
  // data for the entire batch. The different input tensors are not required
  // to be contiguous with each other.
  const auto& req0 = batch->getRequest(0);
  auto inputs0 = req0->getInputs();  // const std::vector<InferenceRequestInput>

  BatchPtr new_batch;
  std::vector<amdinfer::BufferPtr> input_buffers;

  rocalRun(this->handle_);
  // rocalCopyToOutput(this->handle_, mat_input.data, h*w*p);

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
