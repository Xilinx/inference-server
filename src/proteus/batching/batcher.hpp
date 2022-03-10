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
 * @brief Defines the base batcher implementation
 */

#ifndef GUARD_PROTEUS_BATCHING_BATCHER
#define GUARD_PROTEUS_BATCHING_BATCHER

#include <chrono>              // for system_clock::time_point
#include <condition_variable>  // for condition_variable
#include <cstddef>             // for size_t
#include <memory>              // for unique_ptr
#include <mutex>               // for mutex
#include <string>              // for string
#include <thread>              // for thread
#include <vector>              // for vector

#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_LOGGING
#include "proteus/core/interface.hpp"        // IWYU pragma: keep
#include "proteus/helpers/declarations.hpp"  // for BufferPtr, InferenceRequ...
#include "proteus/helpers/queue.hpp"         // for BlockingConcurrentQueue
#include "proteus/observation/logging.hpp"   // for LoggerPtr
#include "proteus/observation/tracing.hpp"   // for SpanPtr

namespace proteus {
class InferenceRequestInput;
class WorkerInfo;
}  // namespace proteus

namespace proteus {

enum class BatcherStatus { kNew, kRun, kInactive, kDead };

/**
 * @brief The Batch is what the batcher produces and pushes to the workers. It
 * represents the requests, the buffers associated with the request and other
 * metadata that should be sent to the worker.
 *
 */
struct Batch {
  std::unique_ptr<std::vector<InferenceRequestPtr>> requests;
  std::unique_ptr<std::vector<BufferPtrs>> input_buffers;
  std::unique_ptr<std::vector<BufferPtrs>> output_buffers;
#ifdef PROTEUS_ENABLE_TRACING
  std::vector<TracePtr> traces;
#endif
#ifdef PROTEUS_ENABLE_METRICS
  std::vector<std::chrono::high_resolution_clock::time_point> start_times;
#endif
};

using BatchPtr = std::unique_ptr<Batch>;
using BatchPtrQueue = BlockingQueue<BatchPtr>;

/**
 * @brief The base batcher implementation defines the basic structure of how
 * batchers behave in proteus. The run() method is purely virtual and must be
 * implemented by the child classes.
 *
 */
class Batcher {
 public:
  /// Construct a new Batcher object
  Batcher();
  explicit Batcher(RequestParameters* parameters);
  /**
   * @brief Construct a new Batcher object
   *
   * @param name the endpoint corresponding to the batcher's worker group
   */
  // explicit Batcher(const std::string& name);
  Batcher(const Batcher& batcher);              ///< copy constructor
  Batcher& operator=(const Batcher&) = delete;  ///< Copy assignment constructor
  Batcher(Batcher&& other) = delete;            ///< Move constructor
  Batcher& operator=(Batcher&& other) =
    delete;                      ///< Move assignment constructor
  virtual ~Batcher() = default;  ///< Destructor

  /**
   * @brief Start the batcher
   *
   * @param worker
   */
  void start(WorkerInfo* worker);
  /**
   * @brief Set the batch size for the batcher
   *
   * @param batch_size target batch size
   */
  void setBatchSize(size_t batch_size);
  /**
   * @brief Set the name of the batcher (i.e. the batcher's worker group
   * endpoint)
   *
   * @param name the endpoint
   */
  void setName(const std::string& name);
  /// Get the batcher's worker group name
  std::string getName();

  /// Get the batcher's input queue (used to enqueue new requests)
  BlockingQueue<InterfacePtr>* getInputQueue();
  /// Get the batcher's output queue (used to push batches to the worker group)
  BatchPtrQueue* getOutputQueue();

  void run(WorkerInfo* worker);

  BatcherStatus getStatus();

  /**
   * @brief Enqueue a new request to the batcher
   *
   * @param request
   */
  void enqueue(InterfacePtr request);

  /// End the batcher
  void end();

 protected:
  size_t batch_size_;
  std::shared_ptr<BlockingQueue<InterfacePtr>> input_queue_;
  std::shared_ptr<BatchPtrQueue> output_queue_;
  std::thread thread_;
  std::condition_variable cv_;
  std::mutex cv_m_;
  std::string model_;
  RequestParameters parameters_;
#ifdef PROTEUS_ENABLE_LOGGING
  LoggerPtr logger_;
#endif

 private:
  /**
   * @brief The doRun method defines the exact process by which the batcher
   * consumes incoming Interface objects and uses them to create batches.
   *
   * @param worker pointer to this batcher's worker [group]
   */
  virtual void doRun(WorkerInfo* worker) = 0;

  BatcherStatus status_;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_BATCHING_BATCHER
