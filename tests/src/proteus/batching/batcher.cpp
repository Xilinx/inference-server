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
 * @brief Implements the fake batcher
 */

#include "proteus/batching/batcher.hpp"

#include <chrono>      // for system_clock::time_point
#include <cstddef>     // for size_t
#include <functional>  // for _Bind_helper<>::type
#include <future>      // for promise
#include <memory>      // for unique_ptr, shared_ptr
#include <stdexcept>   // for invalid_argument
#include <string>      // for operator+
#include <utility>     // for move
#include <vector>      // for vector

#include "proteus/buffers/buffer.hpp"         // IWYU pragma: keep
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/fake_predict_api.hpp"  // for FakeInferenceRequest
#include "proteus/core/interface.hpp"         // for InterfacePtr, Interface
#include "proteus/core/predict_api.hpp"       // for InferenceResponsePromis...
#include "proteus/core/worker_info.hpp"       // for WorkerInfo
#include "proteus/helpers/queue.hpp"          // for BlockingConcurrentQueue
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for Logger
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr

// IWYU pragma: no_forward_declare proteus::Buffer

namespace proteus {

/**
 * @brief For testing purposes, this fake C++ interface returns a
 * fakeInferenceRequest object similar to how the real one does.
 *
 */
class FakeInterface : public Interface {
 public:
  explicit FakeInterface(InferenceRequestInput request);

  std::shared_ptr<InferenceRequest> getRequest(
    const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
    const BufferRawPtrs &output_buffers,
    std::vector<size_t> &output_offsets) override;

  size_t getInputSize() override;
  void errorHandler(const std::exception &e) override;
  std::promise<proteus::InferenceResponse> *getPromise();

 private:
  InferenceRequestInput request_;
  InferenceResponsePromisePtr promise_;
};

FakeInterface::FakeInterface(InferenceRequestInput request)
  : request_(std::move(request)) {
  this->promise_ = std::make_unique<std::promise<proteus::InferenceResponse>>();
}

size_t FakeInterface::getInputSize() { return 1; }

std::promise<proteus::InferenceResponse> *FakeInterface::getPromise() {
  return this->promise_.get();
}

void fakeCppCallback(const InferenceResponsePromisePtr &promise,
                     const InferenceResponse &response) {
  promise->set_value(response);
}

std::shared_ptr<InferenceRequest> FakeInterface::getRequest(
  const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
  const BufferRawPtrs &output_buffers, std::vector<size_t> &output_offsets) {
  auto request = std::make_shared<FakeInferenceRequest>(
    this->request_, input_buffers, input_offsets, output_buffers,
    output_offsets);
  Callback callback =
    std::bind(fakeCppCallback, this->promise_, std::placeholders::_1);
  request->setCallback(std::move(callback));
  return request;
}

void FakeInterface::errorHandler(const std::exception &e) {
  PROTEUS_LOG_ERROR(this->getLogger(), e.what());
  (void)e;  // suppress unused variable warning
}

void Batcher::run(WorkerInfo *worker) {
  auto thread_name = "batch" + this->getName();
  setThreadName(thread_name);
  InterfacePtr req;
  bool run = true;

  while (run) {
    auto batch = std::make_unique<Batch>(worker);
    auto input_buffers = batch->getRawInputBuffers();
    auto output_buffers = batch->getRawOutputBuffers();
    std::vector<size_t> input_offset(input_buffers.size(), 0);
    std::vector<size_t> output_offset(output_buffers.size(), 0);

    // wait for the first request
    this->input_queue_->wait_dequeue(req);
    PROTEUS_LOG_DEBUG(logger_,
                      "Got initial request of a new batch for " + this->model_);

    if (req == nullptr) {
      break;
    }

#ifdef PROTEUS_ENABLE_TRACING
    auto trace = req->getTrace();
    trace->startSpan("fake_batcher");
#endif
    req->getInputSize();  // initialize the req object
    auto new_req = req->getRequest(input_buffers, input_offset, output_buffers,
                                   output_offset);

    batch->addRequest(new_req);

#ifdef PROTEUS_ENABLE_TRACING
    trace->endSpan();
    batch->addTrace(std::move(trace));
#endif
#ifdef PROTEUS_ENABLE_METRICS
    batch->addTime(req->get_time());
#endif
    // batch_size += 1;

    if (!batch->empty()) {
      PROTEUS_LOG_DEBUG(logger_, "Enqueuing batch for " + this->model_);
      this->output_queue_->enqueue(std::move(batch));
    }
  }
}

}  // namespace proteus
