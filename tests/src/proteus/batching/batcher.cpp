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

#include <chrono>              // for system_clock::time_point
#include <condition_variable>  // for condition_variable
#include <cstddef>             // for size_t
#include <functional>          // for _Bind_helper<>::type
#include <future>              // for promise
#include <memory>              // for unique_ptr, shared_ptr
#include <stdexcept>           // for invalid_argument
#include <string>              // for operator+
#include <utility>             // for move
#include <vector>              // for vector

#include "proteus/buffers/buffer.hpp"         // IWYU pragma: keep
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/fake_predict_api.hpp"  // for FakeInferenceRequest
#include "proteus/core/interface.hpp"         // for InterfacePtr, Interface
#include "proteus/core/predict_api.hpp"       // for InferenceResponsePromis...
#include "proteus/core/worker_info.hpp"       // for WorkerInfo
#include "proteus/helpers/queue.hpp"          // for BlockingConcurrentQueue
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_DEBUG
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
    size_t &buffer_index, const std::vector<BufferRawPtrs> &input_buffers,
    std::vector<size_t> &input_offsets,
    const std::vector<BufferRawPtrs> &output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset) override;

  size_t getInputSize() override;
  void errorHandler(const std::invalid_argument &e) override;
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
  size_t &buffer_index, const std::vector<BufferRawPtrs> &input_buffers,
  std::vector<size_t> &input_offsets,
  const std::vector<BufferRawPtrs> &output_buffers,
  std::vector<size_t> &output_offsets, const size_t &batch_size,
  size_t &batch_offset) {
  auto request = std::make_shared<FakeInferenceRequest>(
    this->request_, buffer_index, input_buffers, input_offsets, output_buffers,
    output_offsets, batch_size, batch_offset);
  Callback callback =
    std::bind(fakeCppCallback, this->promise_, std::placeholders::_1);
  request->setCallback(std::move(callback));
  return request;
}

void FakeInterface::errorHandler(const std::invalid_argument &e) {
  SPDLOG_LOGGER_ERROR(this->logger_, e.what());
  (void)e;  // suppress unused variable warning
}

void Batcher::run(WorkerInfo *worker) {
  auto thread_name = "batch" + this->getName();
  setThreadName(thread_name);
  InterfacePtr req;
  bool run = true;

  while (run) {
    auto batch = std::make_unique<Batch>();
    batch->requests =
      std::make_unique<std::vector<std::shared_ptr<InferenceRequest>>>();
    batch->input_buffers = std::make_unique<std::vector<BufferPtrs>>();

    batch->output_buffers = std::make_unique<std::vector<BufferPtrs>>();
    auto input_buffer = worker->getInputBuffer();
    std::vector<size_t> input_offset = {0};
    auto output_buffer = worker->getOutputBuffer();
    std::vector<size_t> output_offset = {0};
    size_t batch_size = 0;

    // wait for the first request
    std::vector<BufferRawPtrs> input_buffers;
    input_buffers.emplace_back();
    for (const auto &buffer : input_buffer) {
      input_buffers.back().push_back(buffer.get());
    }
    std::vector<BufferRawPtrs> output_buffers;
    output_buffers.emplace_back();
    for (const auto &buffer : output_buffer) {
      output_buffers.back().push_back(buffer.get());
    }
    this->input_queue_->wait_dequeue(req);
    SPDLOG_LOGGER_DEBUG(
      this->logger_, "Got initial request of a new batch for " + this->model_);

    if (req == nullptr) {
      break;
    }

#ifdef PROTEUS_ENABLE_TRACING
    auto trace = req->getTrace();
    trace->startSpan("fake_batcher");
#endif
    // auto fake_req = dynamic_cast<FakeInterface*>(req.get());
    // InferenceRequestPtr new_req;
    // if(fake_req != nullptr){
    req->getInputSize();  // initialize the req object
    size_t buffer_index = 0;
    auto new_req =
      req->getRequest(buffer_index, input_buffers, input_offset, output_buffers,
                      output_offset, this->batch_size_, batch_size);
    // } else {
    //   new_req = std::make_unique<FakeInferenceRequest>();
    // }

    batch->requests->push_back(new_req);

#ifdef PROTEUS_ENABLE_TRACING
    trace->endSpan();
    batch->traces.emplace_back(std::move(trace));
#endif
#ifdef PROTEUS_ENABLE_METRICS
    batch->start_times.emplace_back(req->get_time());
#endif
    // batch_size += 1;

    if (!batch->requests->empty()) {
      SPDLOG_LOGGER_DEBUG(this->logger_, "Enqueuing batch for " + this->model_);
      batch->input_buffers->push_back(std::move(input_buffer));
      batch->output_buffers->push_back(std::move(output_buffer));
      this->output_queue_->enqueue(std::move(batch));
    }
  }
}

}  // namespace proteus
