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

#ifndef GUARD_PROTEUS_BATCHING_BATCHER
#define GUARD_PROTEUS_BATCHING_BATCHER

#include <condition_variable>  // for condition_variable
#include <cstddef>             // for size_t
#include <memory>              // for unique_ptr
#include <mutex>               // for mutex
#include <string>              // for string
#include <thread>              // for thread
#include <vector>              // for vector

#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_LOGGING
#include "proteus/core/interface.hpp"        // for InterfacePtr
#include "proteus/helpers/declarations.hpp"  // for BufferPtr, InferenceRequ...
#include "proteus/helpers/queue.hpp"         // for BlockingConcurrentQueue
#include "proteus/observation/logging.hpp"   // for LoggerPtr
#include "proteus/observation/tracing.hpp"   // for SpanPtr

namespace proteus {
class InferenceRequestInput;
class WorkerInfo;
}  // namespace proteus

namespace proteus {

struct Batch {
  std::unique_ptr<std::vector<InferenceRequestPtr>> requests;
  std::unique_ptr<std::vector<BufferPtrs>> input_buffers;
  std::unique_ptr<std::vector<BufferPtrs>> output_buffers;
#ifdef PROTEUS_ENABLE_TRACING
  std::vector<SpanPtr> spans;
#endif
#ifdef PROTEUS_ENABLE_METRICS
  std::vector<std::chrono::high_resolution_clock::time_point> start_times;
#endif
};

using BatchPtr = std::unique_ptr<Batch>;
using BatchPtrQueue = BlockingQueue<BatchPtr>;

class Batcher {
 public:
  Batcher();
  explicit Batcher(const std::string& name);
  Batcher(const Batcher& batcher);              ///< copy constructor
  Batcher& operator=(const Batcher&) = delete;  ///< Copy assignment constructor
  Batcher(Batcher&& other) = delete;            ///< Move constructor
  Batcher& operator=(Batcher&& other) =
    delete;  ///< Move assignment constructor
  virtual ~Batcher() = default;

  void start(WorkerInfo* worker);
  void setBatchSize(size_t batch_size);
  void setName(const std::string& name);
  std::string getName();

  BlockingQueue<InterfacePtr>* getInputQueue();
  BatchPtrQueue* getOutputQueue();

  virtual void run(WorkerInfo* worker) = 0;

  void enqueue(InterfacePtr request);
  InferenceResponseFuture enqueue(InferenceRequestInput request);

  void end();

 protected:
  size_t batch_size_;
  std::shared_ptr<BlockingQueue<InterfacePtr>> input_queue_;
  std::shared_ptr<BatchPtrQueue> output_queue_;
  std::thread thread_;
  std::condition_variable cv_;
  std::mutex cv_m_;
  std::string model_;
#ifdef PROTEUS_ENABLE_LOGGING
  LoggerPtr logger_;
#endif
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_BATCHING_BATCHER
