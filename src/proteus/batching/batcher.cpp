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

#include <memory>   // for shared_ptr, make_sh...
#include <string>   // for string
#include <utility>  // for move

#include "proteus/core/interface.hpp"
#include "proteus/core/predict_api_internal.hpp"  // for RequestParameters
#include "proteus/observation/logging.hpp"        // for getLogger, LoggerPtr
namespace proteus {

/**
 * @brief The C++ Interface class encapsulates incoming requests from the C++
 * API to the batcher.
 *
 */

Batcher::Batcher() {
  this->input_queue_ = std::make_shared<BlockingQueue<InterfacePtr>>();
  this->output_queue_ = std::make_shared<BatchPtrQueue>();
  this->batch_size_ = 1;
  this->status_ = BatcherStatus::kNew;
#ifdef PROTEUS_ENABLE_LOGGING
  this->logger_ = Logger(Loggers::kServer);
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
  this->status_ = BatcherStatus::kNew;
#ifdef PROTEUS_ENABLE_LOGGING
  this->logger_ = Logger(Loggers::kServer);
#endif
  this->model_ = batcher.model_;
}

void Batcher::start(WorkerInfo *worker) {
  this->status_ = BatcherStatus::kRun;
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

void Batcher::run(WorkerInfo *worker) {
  this->doRun(worker);
  this->status_ = BatcherStatus::kInactive;
}

BatcherStatus Batcher::getStatus() { return this->status_; }

void Batcher::end() {
  this->thread_.join();
  this->status_ = BatcherStatus::kDead;
}

#ifdef PROTEUS_ENABLE_LOGGING
const Logger &Batcher::getLogger() const { return logger_; }
#endif

}  // namespace proteus
