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

/**
 * @file
 * @brief Defines the Worker class
 */

#ifndef GUARD_PROTEUS_WORKERS_WORKER
#define GUARD_PROTEUS_WORKERS_WORKER

#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "proteus/batching/soft.hpp"
#include "proteus/buffers/buffer.hpp"
#include "proteus/build_options.hpp"
#include "proteus/core/predict_api.hpp"
#include "proteus/observation/logging.hpp"

namespace proteus {

constexpr auto kNumBufferAuto = -1;

namespace workers {

enum class WorkerStatus {
  kNew,
  kInit,
  kAllocate,
  kAcquire,
  kRun,
  kInactive,
  kRelease,
  kDeallocate,
  kDestroy,
  kDead
};

/**
 * @brief All workers should extend the Worker class which defines the methods
 * to create, start, and run workers over their lifecycle.
 */
class Worker {
 public:
  /// The constructor only initializes the private class members
  Worker(const std::string& name, const std::string& platform)
    : metadata_(name, platform) {
    this->status_ = WorkerStatus::kNew;
#ifdef PROTEUS_ENABLE_LOGGING
    this->logger_ = getLogger();
    if (this->logger_ == nullptr) {
      initLogging();
      this->logger_ = getLogger();
    }
#endif
    this->input_buffers_ = nullptr;
    this->output_buffers_ = nullptr;
    this->max_buffer_num_ = UINT_MAX;
    this->batch_size_ = 1;
  }
  virtual ~Worker() = default;  ///< Destroy the Worker object

  /**
   * @brief Starts the worker's run() method as a separate thread and returns it
   *
   * @param input_queue queue used to send requests to the started worker thread
   * @return std::thread
   */
  virtual std::thread spawn(BatchPtrQueue* input_queue) = 0;

  /// Perform low-cost initialization of the worker
  void init(RequestParameters* parameters) {
    this->status_ = WorkerStatus::kInit;
    this->doInit(parameters);
  }
  /// Allocate some buffers that are used to hold input and output data
  size_t allocate(size_t num) {
    this->status_ = WorkerStatus::kAllocate;
    return this->doAllocate(num);
  }
  /// Acquire any hardware resources or perform high-cost initialization
  void acquire(RequestParameters* parameters) {
    this->status_ = WorkerStatus::kAcquire;
    this->doAcquire(parameters);
    this->metadata_.setReady();
  }
  /**
   * @brief The main body of the worker executes the work
   *
   * @param input_queue queue that receives incoming requests
   */
  void run(BatchPtrQueue* input_queue) {
    this->status_ = WorkerStatus::kRun;
    this->doRun(input_queue);
    this->status_ = WorkerStatus::kInactive;
  }
  /// Release any hardware resources
  void release() {
    this->status_ = WorkerStatus::kRelease;
    this->metadata_.setNotReady();
    this->doRelease();
  }
  /// Free the input and output buffers
  void deallocate() {
    this->status_ = WorkerStatus::kDeallocate;
    this->doDeallocate();
    this->input_buffers_ = nullptr;
    this->output_buffers_ = nullptr;
  }
  /// Perform any final operations before the worker thread is joined
  void destroy() {
    this->status_ = WorkerStatus::kDestroy;
    this->doDestroy();
    this->status_ = WorkerStatus::kDead;
  }

  /**
   * @brief Return buffers to the worker's buffer pool after use. The buffers'
   * reset() method is called prior to returning.
   *
   * @param input_buffers
   * @param output_buffers
   */
  void returnBuffers(std::unique_ptr<std::vector<BufferPtrs>> input_buffers,
                     std::unique_ptr<std::vector<BufferPtrs>> output_buffers) {
    this->input_buffers_->enqueue_bulk(
      std::make_move_iterator(input_buffers->begin()), input_buffers->size());
    this->output_buffers_->enqueue_bulk(
      std::make_move_iterator(output_buffers->begin()), output_buffers->size());
  }

  void setInputBuffers(BufferPtrsQueue* buffers) {
    this->input_buffers_ = buffers;
  }

  void setOutputBuffers(BufferPtrsQueue* buffers) {
    this->output_buffers_ = buffers;
  }

  [[nodiscard]] uint32_t getMaxBufferNum() const {
    return this->max_buffer_num_;
  }

  [[nodiscard]] size_t getBatchSize() const { return this->batch_size_; }
  [[nodiscard]] WorkerStatus getStatus() const { return this->status_; }

  virtual std::vector<std::unique_ptr<Batcher>> makeBatcher(int num = 1) {
    return this->makeBatcher<SoftBatcher>(num);
  }

  template <typename T>
  std::vector<std::unique_ptr<Batcher>> makeBatcher(int num) {
    std::vector<std::unique_ptr<Batcher>> batchers;
    batchers.emplace_back(std::make_unique<T>());
    for (int i = 1; i < num; i++) {
      batchers.push_back(
        std::make_unique<T>(*dynamic_cast<T*>(batchers.back().get())));
    }
    return batchers;
  }

  ModelMetadata getMetadata() { return this->metadata_; }

 protected:
  BufferPtrsQueue* input_buffers_;
  BufferPtrsQueue* output_buffers_;
  uint32_t max_buffer_num_;
#ifdef PROTEUS_ENABLE_LOGGING
  LoggerPtr logger_;
#endif
  size_t batch_size_;
  ModelMetadata metadata_;

 private:
  /// Perform low-cost initialization of the worker
  virtual void doInit(RequestParameters* parameters) = 0;
  /// Allocate some buffers that are used to hold input and output data
  virtual size_t doAllocate(size_t num) = 0;
  /// Acquire any hardware resources or perform high-cost initialization
  virtual void doAcquire(RequestParameters* parameters) = 0;
  /**
   * @brief The main body of the worker executes the work
   *
   * @param input_queue queue that receives incoming requests
   */
  virtual void doRun(BatchPtrQueue* input_queue) = 0;
  /// Release any hardware resources
  virtual void doRelease() = 0;
  /// Free the input and output buffers
  virtual void doDeallocate() = 0;
  /// Perform any final operations before the worker's run thread is joined
  virtual void doDestroy() = 0;

  WorkerStatus status_;
};

}  // namespace workers

}  // namespace proteus

#endif  // GUARD_PROTEUS_WORKERS_WORKER
