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

#include <chrono>   // for system_clock::time_point
#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr, shared_ptr
#include <string>   // for string
#include <thread>   // for thread
#include <vector>   // for vector

#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_LOGGING
#include "proteus/core/predict_api.hpp"      // for RequestParameters
#include "proteus/helpers/declarations.hpp"  // for BufferPtrs, InferenceReq...
#include "proteus/helpers/queue.hpp"         // for BlockingConcurrentQueue
#include "proteus/observation/logging.hpp"   // for LoggerPtr
#include "proteus/observation/tracing.hpp"   // for TracePtr

namespace proteus {
class Buffer;
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
class Batch {
 public:
  explicit Batch(const WorkerInfo* worker);
  ~Batch();

  void addRequest(InferenceRequestPtr request);

  const InferenceRequestPtr& getRequest(int index);
  const std::vector<InferenceRequestPtr>& getRequests();
  const BufferPtrs& getInputBuffers() const;
  const BufferPtrs& getOutputBuffers() const;
  std::vector<Buffer*> getRawInputBuffers() const;
  std::vector<Buffer*> getRawOutputBuffers() const;

  bool empty() const;
  size_t size() const;
  size_t input_size() const;
  size_t output_size() const;

#ifdef PROTEUS_ENABLE_TRACING
  void addTrace(TracePtr trace);
  TracePtr& getTrace(int index);
#endif
#ifdef PROTEUS_ENABLE_METRICS
  void addTime(std::chrono::high_resolution_clock::time_point timestamp);
  std::chrono::high_resolution_clock::time_point getTime(int index);
#endif

  auto begin() const { return requests_.begin(); }
  auto end() const { return requests_.end(); }

 private:
  const WorkerInfo* worker_;
  std::vector<InferenceRequestPtr> requests_;
  std::vector<BufferPtr> input_buffers_;
  std::vector<BufferPtr> output_buffers_;
#ifdef PROTEUS_ENABLE_TRACING
  std::vector<TracePtr> traces_;
#endif
#ifdef PROTEUS_ENABLE_METRICS
  std::vector<std::chrono::high_resolution_clock::time_point> start_times_;
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
#ifdef PROTEUS_ENABLE_LOGGING
  const Logger& getLogger() const;
#endif

  size_t batch_size_;
  std::shared_ptr<BlockingQueue<InterfacePtr>> input_queue_;
  std::shared_ptr<BatchPtrQueue> output_queue_;
  std::thread thread_;
  std::string model_;
  RequestParameters parameters_;

 private:
  /**
   * @brief The doRun method defines the exact process by which the batcher
   * consumes incoming Interface objects and uses them to create batches.
   *
   * @param worker pointer to this batcher's worker [group]
   */
  virtual void doRun(WorkerInfo* worker) = 0;

  BatcherStatus status_;

#ifdef PROTEUS_ENABLE_LOGGING
  Logger logger_{Loggers::kServer};
#endif
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_BATCHING_BATCHER
