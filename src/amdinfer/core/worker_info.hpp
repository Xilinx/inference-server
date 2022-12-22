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
 * @brief Defines the information the Manager saves on each active
 * worker group.
 */

#ifndef GUARD_AMDINFER_CORE_WORKER_INFO
#define GUARD_AMDINFER_CORE_WORKER_INFO

#include <cstddef>  // for size_t
#include <map>      // for map
#include <memory>   // for unique_ptr
#include <string>   // for string
#include <thread>   // for thread, thread::id
#include <vector>   // for vector

#include "amdinfer/declarations.hpp"  // for BufferPtr
#include "amdinfer/util/queue.hpp"    // for BufferPtrsQueuePtr

namespace amdinfer {
class Batcher;
class RequestParameters;
namespace workers {
class Worker;
}  // namespace workers
}  // namespace amdinfer

namespace amdinfer {

/**
 * @brief Stores the metadata associated with a worker. Instances of this class
 * are saved in the Manager.
 */
class WorkerInfo {
 public:
  /// Construct a new WorkerInfo object
  WorkerInfo(const std::string& name, RequestParameters* parameters);
  ~WorkerInfo();                           ///> Destroy a WorkerInfo object
  WorkerInfo(WorkerInfo const&) = delete;  ///< Copy constructor
  /// Copy assignment constructor
  WorkerInfo& operator=(const WorkerInfo&) = delete;
  WorkerInfo(WorkerInfo&& other) = delete;  ///< Move constructor
  /// Move assignment constructor
  WorkerInfo& operator=(WorkerInfo&& other) = delete;

  /**
   * @brief Get the queue associated with this worker to send it requests
   *
   * @return InferenceRequestPtrQueue*
   */
  Batcher* getBatcher();
  /// Blocks until the associated worker's thread joins
  void join(std::thread::id id);
  void joinAll();  ///< Blocks until all workers in the group join

  /**
   * @brief Start a new worker in the group with the given parameters. The name
   * is used to uniquely identify a particular worker to dynamically load.
   *
   * @param name
   * @param parameters pointer to parameters. Should not be nullptr
   */
  void addAndStartWorker(const std::string& name,
                         RequestParameters* parameters);

  /// unload one worker from this worker group
  void unload();
  /// unload all workers from this worker group
  void shutdown();

  /**
   * @brief Get an input buffer from this worker
   *
   * @return BufferPtrs
   */
  [[nodiscard]] BufferPtrs getInputBuffer() const;
  /**
   * @brief Get an output buffer from this worker
   *
   * @return BufferPtrs
   */
  [[nodiscard]] BufferPtrs getOutputBuffer() const;
  /**
   * @brief Return an input buffer to the worker
   *
   * @param buffer the buffer to return
   */
  void putInputBuffer(BufferPtrs buffer) const;
  /**
   * @brief Return an output buffer to the worker
   *
   * @param buffer the buffer to return
   */
  void putOutputBuffer(BufferPtrs buffer) const;

  /**
   * @brief Checks if this worker group supports a particular number of input
   * tensors.
   *
   * @param size
   * @exception invalid_argument too many input tensors for this worker
   * @return true if the number of input tensors is acceptable
   * @return false if more buffers need to be allocated but the number is okay
   */
  [[nodiscard]] bool inputSizeValid(size_t size) const;

  /**
   * @brief Get the number of buffers currently allocated by this worker
   *
   * @return auto
   */
  [[nodiscard]] auto getBufferNum() const { return this->buffer_num_; }

  /**
   * @brief Get the maximum number of buffers that this worker may allocate. -1
   * indicates there is no limit.
   *
   * @return auto
   */
  [[nodiscard]] auto getMaxBufferNum() const { return this->max_buffer_num_; }

  /**
   * @brief Allocate enough buffers to fulfill a particular request size
   *
   * @param request_size
   */
  void allocate(size_t request_size);

  /// get the number of workers in the group
  [[nodiscard]] size_t getGroupSize() const;

  /// get the batch size of the worker group
  [[nodiscard]] auto getBatchSize() const { return this->batch_size_; }

 private:
  std::map<std::thread::id, std::thread> worker_threads_;
  std::map<std::thread::id, workers::Worker*> workers_;
  std::vector<std::unique_ptr<Batcher>> batchers_;
  BufferPtrsQueuePtr input_buffer_ptr_;
  BufferPtrsQueuePtr output_buffer_ptr_;
  size_t buffer_num_ = 0;
  size_t max_buffer_num_ = 0;
  size_t batch_size_ = 1;

  friend class Manager;
};

}  // namespace amdinfer
#endif  // GUARD_AMDINFER_CORE_WORKER_INFO
