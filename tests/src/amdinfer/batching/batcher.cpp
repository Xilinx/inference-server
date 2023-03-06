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
 * @brief Implements the fake batcher
 */

#include "amdinfer/batching/batcher.hpp"

#include <cstddef>    // for size_t
#include <exception>  // for exception
#include <future>     // for promise
#include <memory>     // for shared_ptr, unique_ptr
#include <string>     // for operator+, char_traits
#include <utility>    // for move
#include <vector>     // for vector

#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/fake_predict_api.hpp"  // for FakeInferenceRequest
#include "amdinfer/core/predict_api.hpp"       // for InferenceResponsePromis...
#include "amdinfer/core/protocol_wrapper.hpp"  // for ProtocolWrapper
#include "amdinfer/observation/logging.hpp"    // for Logger, AMDINFER_LOG_DEBUG
#include "amdinfer/observation/tracing.hpp"    // for Trace
#include "amdinfer/util/queue.hpp"             // for BlockingConcurrentQueue
#include "amdinfer/util/thread.hpp"            // for setThreadName

// IWYU pragma: no_forward_declare amdinfer::Buffer

namespace amdinfer {

// /**
//  * @brief For testing purposes, this fake C++ interface returns a
//  * fakeInferenceRequest object similar to how the real one does.
//  *
//  */
// class FakeProtocolWrapper : public ProtocolWrapper {
//  public:
//   explicit FakeProtocolWrapper(InferenceRequestInput request);

//   std::shared_ptr<InferenceRequest> getRequest(
//     const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
//     const BufferRawPtrs &output_buffers,
//     std::vector<size_t> &output_offsets) override;

//   size_t getInputSize() override;
//   void errorHandler(const std::exception &e) override;
//   std::promise<amdinfer::InferenceResponse> *getPromise();

//  private:
//   InferenceRequestInput request_;
//   InferenceResponsePromisePtr promise_;
// };

// FakeProtocolWrapper::FakeProtocolWrapper(InferenceRequestInput request)
//   : request_(std::move(request)) {
//   this->promise_ =
//     std::make_unique<std::promise<amdinfer::InferenceResponse>>();
// }

// size_t FakeProtocolWrapper::getInputSize() { return 1; }

// std::promise<amdinfer::InferenceResponse> *FakeProtocolWrapper::getPromise()
// {
//   return this->promise_.get();
// }

// void fakeCppCallback(const InferenceResponsePromisePtr &promise,
//                      const InferenceResponse &response) {
//   promise->set_value(response);
// }

// std::shared_ptr<InferenceRequest> FakeProtocolWrapper::getRequest(
//   const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
//   const BufferRawPtrs &output_buffers, std::vector<size_t> &output_offsets) {
//   auto request = std::make_shared<FakeInferenceRequest>(
//     this->request_, input_buffers, input_offsets, output_buffers,
//     output_offsets);
//   Callback callback = [this](const InferenceResponse &response) {
//     fakeCppCallback(this->promise_, response);
//   };
//   request->setCallback(std::move(callback));
//   return request;
// }

// void FakeProtocolWrapper::errorHandler(const std::exception &e) {
//   AMDINFER_LOG_ERROR(this->getLogger(), e.what());
//   (void)e;  // suppress unused variable warning
// }

// void Batcher::run(WorkerInfo *worker) {
//   auto thread_name = "batch" + this->getName();
//   util::setThreadName(thread_name);
//   ProtocolWrapperPtr req;
//   bool run = true;

//   while (run) {
//     auto batch = std::make_unique<Batch>(worker);
//     auto input_buffers = batch->getRawInputBuffers();
//     auto output_buffers = batch->getRawOutputBuffers();
//     std::vector<size_t> input_offset(input_buffers.size(), 0);
//     std::vector<size_t> output_offset(output_buffers.size(), 0);

//     // wait for the first request
//     this->input_queue_->wait_dequeue(req);
//     AMDINFER_LOG_DEBUG(
//       logger_, "Got initial request of a new batch for " + this->model_);

//     if (req == nullptr) {
//       break;
//     }

// #ifdef AMDINFER_ENABLE_TRACING
//     auto trace = req->getTrace();
//     trace->startSpan("fake_batcher");
// #endif
//     req->getInputSize();  // initialize the req object
//     auto new_req = req->getRequest(input_buffers, input_offset,
//     output_buffers,
//                                    output_offset);

//     batch->addRequest(new_req);

// #ifdef AMDINFER_ENABLE_TRACING
//     trace->endSpan();
//     batch->addTrace(std::move(trace));
// #endif
// #ifdef AMDINFER_ENABLE_METRICS
//     batch->addTime(req->getTime());
// #endif
//     // batch_size += 1;

//     if (!batch->empty()) {
//       AMDINFER_LOG_DEBUG(logger_, "Enqueuing batch for " + this->model_);
//       this->output_queue_->enqueue(std::move(batch));
//     }
//   }
// }

}  // namespace amdinfer
