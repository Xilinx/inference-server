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

/**
 * @file
 * @brief Defines the Worker class
 */

#ifndef GUARD_AMDINFER_WORKERS_WORKER
#define GUARD_AMDINFER_WORKERS_WORKER

#include <cassert>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "amdinfer/batching/soft.hpp"
#include "amdinfer/buffers/buffer.hpp"
#include "amdinfer/build_options.hpp"
#include "amdinfer/core/memory_pool/pool.hpp"
#include "amdinfer/core/model_metadata.hpp"
#include "amdinfer/observation/logging.hpp"
#include "amdinfer/observation/metrics.hpp"
#include "amdinfer/util/ctpl.hpp"    // for ThreadPool
#include "amdinfer/util/thread.hpp"  // for setThreadName

namespace amdinfer {

constexpr auto kNumBufferAuto = -1;

namespace workers {

enum class WorkerStatus {
  New,
  Init,
  Acquire,
  Run,
  Inactive,
  Release,
  Destroy,
  Dead
};

/**
 * @brief All workers should extend the Worker class which defines the methods
 * to create, start, and run workers over their lifecycle.
 */
class Worker {
 public:
  /// The constructor only initializes the private class members
  Worker(const std::string& name, const std::string& platform, bool allow_next)
    : metadata_(name, platform), allow_next_(allow_next) {
    this->status_ = WorkerStatus::New;
  }
  virtual ~Worker() = default;  ///< Destroy the Worker object

  /// Get the memory allocators supported by this worker
  [[nodiscard]] virtual std::vector<MemoryAllocators> getAllocators() const = 0;

  /// Perform low-cost initialization of the worker
  void init(ParameterMap* parameters) {
    this->status_ = WorkerStatus::Init;
    this->doInit(parameters);
  }
  /// Acquire any hardware resources or perform high-cost initialization
  void acquire(ParameterMap* parameters) {
    this->status_ = WorkerStatus::Acquire;
    this->doAcquire(parameters);
    this->metadata_.setReady(true);
  }
  /**
   * @brief The main body of the worker executes the work
   *
   * @param input_queue queue that receives incoming requests
   */
  virtual void run(BatchPtrQueue* input_queue, const MemoryPool* pool) = 0;
  /// Release any hardware resources
  void release() {
    status_ = WorkerStatus::Release;
    metadata_.setReady(false);
    this->doRelease();
  }
  /// Perform any final operations before the worker thread is joined
  void destroy() {
    this->status_ = WorkerStatus::Destroy;
    this->doDestroy();
    this->status_ = WorkerStatus::Dead;
  }

  [[nodiscard]] size_t getBatchSize() const { return this->batch_size_; }
  [[nodiscard]] WorkerStatus getStatus() const { return this->status_; }

  virtual std::vector<std::unique_ptr<Batcher>> makeBatcher(
    int num, ParameterMap* parameters, MemoryPool* pool) {
    return this->makeBatcher<SoftBatcher>(num, parameters, pool);
  }

  template <typename T>
  std::vector<std::unique_ptr<Batcher>> makeBatcher(int num,
                                                    ParameterMap* parameters,
                                                    MemoryPool* pool) {
    std::vector<std::unique_ptr<Batcher>> batchers;
    batchers.emplace_back(std::make_unique<T>(pool, parameters));
    for (int i = 1; i < num; i++) {
      batchers.push_back(
        std::make_unique<T>(*dynamic_cast<T*>(batchers.back().get())));
    }
    return batchers;
  }

  ModelMetadata getMetadata() const { return this->metadata_; }

  const std::string& getName() const { return metadata_.getName(); }

  void setNext(BatchPtrQueue* batcher) {
    if (allow_next_) {
      next_ = batcher;
    }
  }
  void setNextAllocators(const std::vector<MemoryAllocators>& allocators) {
    next_allocators_ = allocators;
  }

 protected:
#ifdef AMDINFER_ENABLE_LOGGING
  [[nodiscard]] const Logger& getLogger() const { return logger_; };
#endif

  size_t batch_size_ = 1;
  ModelMetadata metadata_;
  std::vector<MemoryAllocators> next_allocators_;
  BatchPtrQueue* next_ = nullptr;
  WorkerStatus status_;

  /**
   * @brief The main body of the worker executes the work
   *
   * @param input_queue queue that receives incoming requests
   */
  virtual std::unique_ptr<Batch> doRun(Batch* batch,
                                       const MemoryPool* pool) = 0;

 private:
  /// Perform low-cost initialization of the worker
  virtual void doInit(ParameterMap* parameters) = 0;
  /// Acquire any hardware resources or perform high-cost initialization
  virtual void doAcquire(ParameterMap* parameters) = 0;
  /// Release any hardware resources
  virtual void doRelease() = 0;
  /// Perform any final operations before the worker's run thread is joined
  virtual void doDestroy() = 0;

#ifdef AMDINFER_ENABLE_LOGGING
  Logger logger_{Loggers::Server};
#endif
  bool allow_next_ = true;
};

class SingleThreadedWorker : public Worker {
 public:
  using Worker::Worker;
  /**
   * @brief The main body of the worker executes the work
   *
   * @param input_queue queue that receives incoming requests
   */
  void run(BatchPtrQueue* input_queue, const MemoryPool* pool) override {
    this->status_ = WorkerStatus::Run;
    const auto& name = this->getName();
    AMDINFER_IF_LOGGING(const auto logger = this->getLogger();)
    util::setThreadName(name);

    while (true) {
      BatchPtr batch;
      input_queue->wait_dequeue(batch);
      if (batch == nullptr) {
        break;
      }

      [[maybe_unused]] auto batch_size = batch->size();

#ifdef AMDINFER_ENABLE_TRACING
      for (auto i = 0U; i < batch_size; ++i) {
        const auto& trace = batch->getTrace(i);
        trace->startSpan(name.c_str());
      }
#endif

      AMDINFER_LOG_INFO(logger, "Got request in " + name);
#ifdef AMDINFER_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::PipelineIngressWorker);
#endif

      auto new_batch = this->doRun(batch.get(), pool);

      if (next_ != nullptr) {
        assert(new_batch != nullptr);
        assert(new_batch->size() == batch_size);
#ifdef AMDINFER_ENABLE_TRACING
        for (auto i = 0U; i < batch_size; ++i) {
          auto& trace = batch->getTrace(i);
          trace->endSpan();
          new_batch->addTrace(std::move(trace));
        }
#endif
        next_->enqueue(std::move(new_batch));
      }

      const auto& buffers = batch->getInputBuffers();
      for (const auto& buffer : buffers) {
        buffer->free();
      }
    }

    AMDINFER_LOG_INFO(logger, name + " ending");

    status_ = WorkerStatus::Inactive;
  }

 private:
  using Worker::next_;
  using Worker::status_;
};

class MultiThreadedWorker : public Worker {
 public:
  using Worker::Worker;
  /**
   * @brief The main body of the worker executes the work
   *
   * @param input_queue queue that receives incoming requests
   */
  void run(BatchPtrQueue* input_queue, const MemoryPool* pool) override {
    this->status_ = WorkerStatus::Run;
    const auto& name = this->getName();
    AMDINFER_IF_LOGGING(const auto logger = this->getLogger());
    util::setThreadName(name);
    std::atomic_int32_t outstanding_batches = 0;
    // 4 is arbitrary
    const int max_outstanding_batches = thread_pool_.getSize() * 4;
    // 10 is arbitrary
    const auto thread_pool_delay = std::chrono::milliseconds(10);

    while (true) {
      BatchPtr batch;
      input_queue->wait_dequeue(batch);
      if (batch == nullptr) {
        break;
      }

      [[maybe_unused]] auto batch_size = batch->size();

#ifdef AMDINFER_ENABLE_TRACING
      for (auto i = 0U; i < batch_size; ++i) {
        const auto& trace = batch->getTrace(i);
        trace->startSpan(name.c_str());
      }
#endif

      AMDINFER_LOG_INFO(logger, "Got request in " + name);
#ifdef AMDINFER_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::PipelineIngressWorker);
#endif

      outstanding_batches++;
      if (outstanding_batches > max_outstanding_batches) {
        std::this_thread::sleep_for(thread_pool_delay);
      }

      thread_pool_.push([this, batch = std::move(batch), &outstanding_batches,
                         pool]([[maybe_unused]] int id) {
        [[maybe_unused]] auto batch_size = batch->size();
        auto new_batch = this->doRun(batch.get(), pool);

        if (next_ != nullptr) {
          assert(new_batch != nullptr);
          assert(new_batch->size() == batch_size);
#ifdef AMDINFER_ENABLE_TRACING
          for (auto i = 0U; i < batch_size; ++i) {
            auto& trace = batch->getTrace(i);
            trace->endSpan();
            new_batch->addTrace(std::move(trace));
          }
#endif

#ifdef AMDINFER_ENABLE_METRICS
          for (auto i = 0U; i < batch_size; ++i) {
            new_batch->addTime(batch->getTime(i));
          }
#endif

          next_->enqueue(std::move(new_batch));
        }

        const auto& buffers = batch->getInputBuffers();
        for (const auto& buffer : buffers) {
          buffer->free();
        }

        outstanding_batches--;
      });
    }

    AMDINFER_LOG_INFO(logger, name + " ending");

    status_ = WorkerStatus::Inactive;
  }

 protected:
  void createThreadPool(int threads) { thread_pool_.resize(threads); }

  void destroyThreadPool() { thread_pool_.stop(true); }

 private:
  using Worker::next_;
  using Worker::status_;
  util::ThreadPool thread_pool_;
};

}  // namespace workers

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_WORKERS_WORKER
