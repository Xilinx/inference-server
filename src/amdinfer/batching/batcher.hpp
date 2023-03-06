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
 * @brief Defines the base batcher implementation
 */

#ifndef GUARD_AMDINFER_BATCHING_BATCHER
#define GUARD_AMDINFER_BATCHING_BATCHER

#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr, shared_ptr
#include <string>   // for string
#include <thread>   // for thread
#include <vector>   // for vector

#include "amdinfer/batching/batch.hpp"       // for Batch
#include "amdinfer/build_options.hpp"        // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/core/parameters.hpp"      // for ParameterMap
#include "amdinfer/declarations.hpp"         // for BufferPtrs, InferenceReq...
#include "amdinfer/observation/logging.hpp"  // for LoggerPtr
#include "amdinfer/observation/tracing.hpp"  // for TracePtr
#include "amdinfer/util/queue.hpp"           // for BlockingConcurrentQueue

namespace amdinfer {
class Buffer;
class WorkerInfo;
class MemoryPool;
enum class MemoryAllocators;
}  // namespace amdinfer

namespace amdinfer {

enum class BatcherStatus { New, Run, Inactive, Dead };

using BatchPtrQueue = BlockingQueue<BatchPtr>;

/**
 * @brief The base batcher implementation defines the basic structure of how
 * batchers behave in amdinfer. The run() method is purely virtual and must be
 * implemented by the child classes.
 *
 */
class Batcher {
 public:
  /// Construct a new Batcher object
  explicit Batcher(MemoryPool* pool);
  Batcher(MemoryPool* pool, ParameterMap* parameters);
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
  void start(const std::vector<MemoryAllocators>& allocator);
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
  [[nodiscard]] std::string getName() const;

  /// Get the batcher's input queue (used to enqueue new requests)
  BlockingQueue<ProtocolWrapperPtr>* getInputQueue();
  /// Get the batcher's output queue (used to push batches to the worker group)
  BatchPtrQueue* getOutputQueue();

  void run(const std::vector<MemoryAllocators>& allocators);

  BatcherStatus getStatus() const;

  /**
   * @brief Enqueue a new request to the batcher
   *
   * @param request
   */
  void enqueue(ProtocolWrapperPtr request) const;

  /// End the batcher
  void end();

 protected:
#ifdef AMDINFER_ENABLE_LOGGING
  [[nodiscard]] const Logger& getLogger() const;
#endif

  size_t batch_size_ = 1;
  std::shared_ptr<BlockingQueue<ProtocolWrapperPtr>> input_queue_;
  std::shared_ptr<BatchPtrQueue> output_queue_;
  std::thread thread_;
  std::string model_;
  ParameterMap parameters_;
  MemoryPool* pool_;

 private:
  /**
   * @brief The doRun method defines the exact process by which the batcher
   * consumes incoming ProtocolWrapper objects and uses them to create batches.
   *
   * @param allocators vector of allocators that may be used to get memory
   */
  virtual void doRun(const std::vector<MemoryAllocators>& allocators) = 0;

  BatcherStatus status_;

#ifdef AMDINFER_ENABLE_LOGGING
  Logger logger_{Loggers::Server};
#endif
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BATCHING_BATCHER
