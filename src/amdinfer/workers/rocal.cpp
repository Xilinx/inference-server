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
 * @brief The rocAL worker 
 *
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

  // the folder path to be loaded.
  std::filesystem::path folder_path_;
  
  // The context for an augmentation pipeline. Contains most of
  // the worker's important info such as number, data types and sizes of
  // input and output buffers
  RocalContext handle_;

};

std::vector<MemoryAllocators> RocalWorker::getAllocators() const {
  return {MemoryAllocators::Cpu};
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

  AMDINFER_LOG_INFO(logger, " RocalWorker::doInit \n");

  if (parameters->has("batch")) {
    this->batch_size_ = parameters->get<int>("batch");
  }
  if (parameters->has("folder")) {
    this->folder_path_ = parameters->get<std::string>("folder");
  }

  this->handle_ = rocalCreate(batch_size_, RocalProcessMode::ROCAL_PROCESS_CPU, 0, 1);

  if (rocalGetStatus(this->handle_) != ROCAL_OK) {
    AMDINFER_LOG_ERROR(
      logger, "RocalWorker handle creation failed");
    // Throwing an exception causes server to delete this worker instance.
    // Client must try again.
    throw std::invalid_argument(
      "could not create rocAL context");
  }

  int shard_count = 0;
  int width = 224, height = 224;
  RocalImage input = rocalJpegFileSource(handle_, folder_path_.c_str(), RocalImageColor::ROCAL_COLOR_RGB24, shard_count, false, false, false, ROCAL_USE_USER_GIVEN_SIZE, width, height);
  [[maybe_unused]]RocalImage image1 = rocalResize(handle_, input, width, height, true);

  if(rocalGetStatus(this->handle_) != ROCAL_OK)
    {
      std::cout << "JPEG source could not initialize : "<<rocalGetErrorMessage(this->handle_) << std::endl;
    }
  // Calling the API to verify and build the augmentation graph
  if(rocalVerify(handle_) != ROCAL_OK) {
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
