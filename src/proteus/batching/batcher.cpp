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
 * @brief Implements the base batcher and CppNativeApi interface
 */

#include "proteus/batching/batcher.hpp"

#include <functional>  // for _Bind_helper<>::type, _Pl...
#include <future>      // for promise
#include <memory>      // for shared_ptr, make_unique
#include <stdexcept>   // for invalid_argument
#include <string>      // for string
#include <utility>     // for move

#include "proteus/core/interface.hpp"  // for InterfacePtr, Interface
#include "proteus/core/predict_api_internal.hpp"
#include "proteus/core/worker_info.hpp"     // for WorkerInfo
#include "proteus/observation/logging.hpp"  // for LoggerPtr, SPDLOG_LOGGER_...

// IWYU pragma: no_forward_declare proteus::Buffer

namespace proteus {

/**
 * @brief The C++ Interface class encapsulates incoming requests from the C++
 * API to the batcher.
 *
 */
class CppNativeApi : public Interface {
 public:
  explicit CppNativeApi(InferenceRequest request);

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
  InferenceRequest request_;
  InferenceResponsePromisePtr promise_;
};

CppNativeApi::CppNativeApi(InferenceRequest request)
  : request_(std::move(request)) {
  this->promise_ = std::make_unique<std::promise<proteus::InferenceResponse>>();
}

size_t CppNativeApi::getInputSize() { return this->request_.getInputSize(); }

std::promise<proteus::InferenceResponse> *CppNativeApi::getPromise() {
  return this->promise_.get();
}

void cppCallback(const InferenceResponsePromisePtr &promise,
                 const InferenceResponse &response) {
  promise->set_value(response);
}

std::shared_ptr<InferenceRequest> CppNativeApi::getRequest(
  size_t &buffer_index, const std::vector<BufferRawPtrs> &input_buffers,
  std::vector<size_t> &input_offsets,
  const std::vector<BufferRawPtrs> &output_buffers,
  std::vector<size_t> &output_offsets, const size_t &batch_size,
  size_t &batch_offset) {
  auto request = InferenceRequestBuilder::fromInput(
    this->request_, buffer_index, input_buffers, input_offsets, output_buffers,
    output_offsets, batch_size, batch_offset);
  Callback callback =
    std::bind(cppCallback, this->promise_, std::placeholders::_1);
  request->setCallback(std::move(callback));
  return request;
}

void CppNativeApi::errorHandler(const std::invalid_argument &e) {
  SPDLOG_LOGGER_ERROR(this->logger_, e.what());
  this->getPromise()->set_value(InferenceResponse(e.what()));
}

Batcher::Batcher() {
  this->input_queue_ = std::make_shared<BlockingQueue<InterfacePtr>>();
  this->output_queue_ = std::make_shared<BatchPtrQueue>();
  this->batch_size_ = 1;
#ifdef PROTEUS_ENABLE_LOGGING
  this->logger_ = getLogger();
#endif
}

Batcher::Batcher(RequestParameters *parameters) : Batcher() {
  if (parameters != nullptr) {
    this->parameters_ = *parameters;
  }
}

Batcher::Batcher(const Batcher &batcher) {
  this->input_queue_ = batcher.input_queue_;
  this->output_queue_ = batcher.output_queue_;
  this->batch_size_ = batcher.batch_size_;
#ifdef PROTEUS_ENABLE_LOGGING
  this->logger_ = getLogger();
#endif
  this->model_ = batcher.model_;
}

void Batcher::start(WorkerInfo *worker) {
  this->thread_ = std::thread(&Batcher::run, this, worker);
}

void Batcher::setBatchSize(size_t batch_size) {
  this->batch_size_ = batch_size;
}

void Batcher::setName(const std::string &name) { this->model_ = name; }

std::string Batcher::getName() { return this->model_; }

BlockingQueue<InterfacePtr> *Batcher::getInputQueue() {
  return this->input_queue_.get();
}

BatchPtrQueue *Batcher::getOutputQueue() { return this->output_queue_.get(); }

void Batcher::enqueue(InterfacePtr request) {
  this->input_queue_->enqueue(std::move(request));
  this->cv_.notify_one();
}

InferenceResponseFuture Batcher::enqueue(InferenceRequest request) {
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(__func__);
  trace->startSpan("C++ enqueue");
#endif
  auto api = std::make_unique<CppNativeApi>(std::move(request));
  auto future = api->getPromise()->get_future();
#ifdef PROTEUS_ENABLE_TRACING
  trace->endSpan();
  api->setTrace(std::move(trace));
#endif
  this->input_queue_->enqueue(std::move(api));
  this->cv_.notify_one();
  return future;
}

void Batcher::end() {
  this->enqueue(nullptr);
  this->thread_.join();
}

}  // namespace proteus
