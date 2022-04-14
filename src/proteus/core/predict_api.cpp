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
 * @brief Implements the objects used to hold incoming inference requests
 */

#include "proteus/core/predict_api.hpp"

#include <algorithm>  // for copy
#include <iterator>   // for back_insert_iterator, back_inse...
#include <numeric>    // for accumulate
#include <utility>    // for pair, make_pair, move

#include "proteus/build_options.hpp"  // for PROTEUS_ENABLE_TRACING

namespace proteus {

using types::DataType;

void RequestParameters::put(const std::string &key, bool value) {
  this->parameters_.insert(std::make_pair(key, value));
}

void RequestParameters::put(const std::string &key, double value) {
  this->parameters_.insert(std::make_pair(key, value));
}

void RequestParameters::put(const std::string &key, int32_t value) {
  this->parameters_.insert(std::make_pair(key, value));
}

void RequestParameters::put(const std::string &key, const std::string &value) {
  this->parameters_.insert(std::make_pair(key, value));
}

void RequestParameters::put(const std::string &key, const char *value) {
  this->parameters_.insert(std::make_pair(key, std::string(value)));
}

void RequestParameters::erase(const std::string &key) {
  if (this->parameters_.find(key) != this->parameters_.end()) {
    this->parameters_.erase(key);
  }
}

bool RequestParameters::has(const std::string &key) {
  return this->parameters_.find(key) != this->parameters_.end();
}

size_t RequestParameters::size() const { return parameters_.size(); }

bool RequestParameters::empty() const { return parameters_.empty(); }

std::map<std::string, Parameter> RequestParameters::data() const {
  return parameters_;
}

void InferenceRequest::setCallback(Callback &&callback) {
  callback_ = callback;
}

void InferenceRequest::runCallbackOnce(const InferenceResponse &response) {
  if (this->callback_ != nullptr) {
    (this->callback_)(response);
    this->callback_ = nullptr;
  }
}

void InferenceRequest::runCallback(const InferenceResponse &response) {
  (this->callback_)(response);
}

void InferenceRequest::runCallbackError(std::string_view error_msg) {
  this->runCallback(InferenceResponse(std::string{error_msg}));
}

void InferenceRequest::addInputTensor(void *data, std::vector<uint64_t> shape,
                                      types::DataType dataType,
                                      std::string name) {
  this->inputs_.emplace_back(data, shape, dataType, name);
}

void InferenceRequest::addInputTensor(InferenceRequestInput input) {
  this->inputs_.push_back(input);
}

const std::vector<InferenceRequestInput> &InferenceRequest::getInputs() const {
  return this->inputs_;
}

size_t InferenceRequest::getInputSize() { return this->inputs_.size(); }

const std::vector<InferenceRequestOutput> &InferenceRequest::getOutputs()
  const {
  return this->outputs_;
}

void InferenceRequest::addOutputTensor(InferenceRequestOutput output) {
  this->outputs_.push_back(output);
}

InferenceRequestInput::InferenceRequestInput(void *data,
                                             std::vector<uint64_t> shape,
                                             types::DataType dataType,
                                             std::string name) {
  this->data_ = data;
  this->shared_data_ = nullptr;
  this->shape_ = std::move(shape);
  this->dataType_ = dataType;
  this->name_ = std::move(name);
  this->parameters_ = std::make_unique<RequestParameters>();
}

InferenceRequestInput::InferenceRequestInput() {
  this->data_ = nullptr;
  this->shared_data_ = nullptr;
  this->name_ = "";
  this->dataType_ = DataType::UINT32;
  this->parameters_ = std::make_unique<RequestParameters>();
}

void InferenceRequestInput::setName(std::string name) {
  this->name_ = std::move(name);
}

void InferenceRequestInput::setDatatype(types::DataType type) {
  this->dataType_ = type;
}

size_t InferenceRequestInput::getSize() const {
  return std::accumulate(this->shape_.begin(), this->shape_.end(), 1,
                         std::multiplies<>());
}

void *InferenceRequestInput::getData() const {
  if (this->shared_data_ != nullptr) {
    return this->shared_data_.get();
  }
  return this->data_;
}

InferenceRequestOutput::InferenceRequestOutput() {
  this->data_ = nullptr;
  this->name_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
}

void InferenceRequestOutput::setName(const std::string &name) {
  this->name_ = name;
}

InferenceResponse::InferenceResponse() {
  this->parameters_ = std::make_unique<RequestParameters>();
}

InferenceResponse::InferenceResponse(const std::string &error_msg) {
  this->parameters_ = nullptr;
  this->error_msg_ = error_msg;
}

void InferenceResponse::setID(const std::string &id) { this->id_ = id; }

void InferenceResponse::setModel(const std::string &model) {
  this->model_ = model;
}

std::string InferenceResponse::getModel() { return this->model_; }

bool InferenceResponse::isError() const { return !this->error_msg_.empty(); }

std::string InferenceResponse::getError() const { return this->error_msg_; }

void InferenceResponse::addOutput(const InferenceResponseOutput &output) {
  this->outputs_.push_back(output);
}

std::vector<InferenceResponseOutput> InferenceResponse::getOutputs() const {
  return this->outputs_;
}

#ifdef PROTEUS_ENABLE_TRACING
void InferenceResponse::setContext(StringMap &&context) {
  this->context_ = std::move(context);
}

const StringMap &InferenceResponse::getContext() const {
  return this->context_;
}
#endif

ModelMetadataTensor::ModelMetadataTensor(const std::string &name,
                                         types::DataType datatype,
                                         std::vector<uint64_t> shape) {
  this->name_ = name;
  this->datatype_ = datatype;
  this->shape_ = std::move(shape);
}

const std::string &ModelMetadataTensor::getName() const { return this->name_; }

const types::DataType &ModelMetadataTensor::getDataType() const {
  return this->datatype_;
}
const std::vector<uint64_t> &ModelMetadataTensor::getShape() const {
  return this->shape_;
}

ModelMetadata::ModelMetadata(const std::string &name,
                             const std::string &platform) {
  this->name_ = name;
  this->platform_ = platform;
  this->ready_ = false;
}

void ModelMetadata::addInputTensor(const std::string &name,
                                   types::DataType datatype,
                                   std::initializer_list<uint64_t> shape) {
  this->inputs_.emplace_back(name, datatype, shape);
}

void ModelMetadata::addInputTensor(const std::string &name,
                                   types::DataType datatype,
                                   std::vector<int> shape) {
  std::vector<uint64_t> new_shape;
  std::copy(shape.begin(), shape.end(), std::back_inserter(new_shape));
  this->inputs_.emplace_back(name, datatype, new_shape);
}

void ModelMetadata::addOutputTensor(const std::string &name,
                                    types::DataType datatype,
                                    std::initializer_list<uint64_t> shape) {
  this->outputs_.emplace_back(name, datatype, shape);
}

void ModelMetadata::addOutputTensor(const std::string &name,
                                    types::DataType datatype,
                                    std::vector<int> shape) {
  std::vector<uint64_t> new_shape;
  std::copy(shape.begin(), shape.end(), std::back_inserter(new_shape));
  this->outputs_.emplace_back(name, datatype, new_shape);
}

const std::string &ModelMetadata::getName() const { return this->name_; }
void ModelMetadata::setName(const std::string &name) { this->name_ = name; }

void ModelMetadata::setReady(bool ready) { this->ready_ = ready; }

bool ModelMetadata::isReady() const { return this->ready_; }

const std::string &ModelMetadata::getPlatform() const {
  return this->platform_;
}

const std::vector<ModelMetadataTensor> &ModelMetadata::getInputs() const {
  return this->inputs_;
}

const std::vector<ModelMetadataTensor> &ModelMetadata::getOutputs() const {
  return this->outputs_;
}

}  // namespace proteus
