// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief
 */

#include "amdinfer/core/inference_request.hpp"

#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/util/containers.hpp"          // for containerProduct
#include "amdinfer/util/memory.hpp"              // for copy

namespace amdinfer {

InferenceRequestPtr InferenceRequest::propagate() {
  auto new_request = std::make_shared<InferenceRequest>();
  new_request->setCallback(this->getCallback());
  new_request->setID(this->getID());
  const auto outputs = this->getOutputs();
  for (const auto &output : outputs) {
    new_request->addOutputTensor(output);
  }

  return new_request;
}

void InferenceRequest::setCallback(Callback &&callback) {
  callback_ = std::move(callback);
}

Callback InferenceRequest::getCallback() { return std::move(callback_); }

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

void InferenceRequest::addInputTensor(void *data,
                                      const std::vector<int64_t> &shape,
                                      DataType data_type,
                                      const std::string &name) {
  this->inputs_.emplace_back(data, shape, data_type, name);
}

void InferenceRequest::addInputTensor(InferenceRequestInput input) {
  this->inputs_.push_back(std::move(input));
}

void InferenceRequest::setInputTensorData(size_t index, void *data) {
  if (index < inputs_.size()) {
    auto &input = inputs_.at(index);
    input.setData(data);
  }
}

const std::vector<InferenceRequestInput> &InferenceRequest::getInputs() const {
  return this->inputs_;
}

size_t InferenceRequest::getInputSize() const { return this->inputs_.size(); }

const std::vector<InferenceRequestOutput> &InferenceRequest::getOutputs()
  const {
  return this->outputs_;
}

void InferenceRequest::addOutputTensor(const InferenceRequestOutput &output) {
  this->outputs_.push_back(output);
}

InferenceRequestInput::InferenceRequestInput(void *data,
                                             std::vector<int64_t> shape,
                                             DataType data_type,
                                             std::string name)
  : InferenceTensor(std::move(name), std::move(shape), data_type),
    data_(data) {}

InferenceRequestInput::InferenceRequestInput(const Tensor &tensor)
  : InferenceTensor(tensor) {}

InferenceRequestInput::InferenceRequestInput()
  : InferenceTensor("", {}, DataType::Unknown) {}

void InferenceRequestInput::setData(void *buffer) { this->data_ = buffer; }

void *InferenceRequestInput::getData() const { return this->data_; }

struct InferenceRequestInputSizes {
  size_t data;
};

size_t InferenceRequestInput::serializeSize() const {
  auto size = InferenceTensor::serializeSize();
  size += sizeof(InferenceRequestInputSizes);
  size += this->getSize() * this->getDatatype().size();
  return size;
}

std::byte *InferenceRequestInput::serialize(std::byte *data_out) const {
  auto *data = data_out;
  data = InferenceTensor::serialize(data);

  InferenceRequestInputSizes metadata{this->getSize() *
                                      this->getDatatype().size()};
  data = util::copy(metadata, data, sizeof(InferenceRequestInputSizes));
  data = util::copy(data_, data, metadata.data);
  assert(data_out + this->serializeSize() == data);
  return data;
}

const std::byte *InferenceRequestInput::deserialize(const std::byte *data_in) {
  data_in = InferenceTensor::deserialize(data_in);

  const auto metadata =
    *reinterpret_cast<const InferenceRequestInputSizes *>(data_in);
  data_in += sizeof(InferenceRequestInputSizes);

  return util::copy(data_in, static_cast<std::byte *>(data_), metadata.data);
}

std::ostream &operator<<(std::ostream &os, InferenceRequestInput const &self) {
  os << "InferenceRequestInput:\n";
  os << "  Name: " << self.getName() << "\n";
  os << "  Shape: ";
  for (const auto &index : self.getShape()) {
    os << index << ",";
  }
  os << "\n";
  os << "  Datatype: " << self.getDatatype().str() << "\n";
  os << "  Parameters:\n";
  os << self.getParameters() << "\n";
  os << "  Data: " << self.getData() << "\n";
  return os;
}

InferenceRequestOutput::InferenceRequestOutput() {
  this->data_ = nullptr;
  this->name_ = "";
}

void InferenceRequestOutput::setName(const std::string &name) {
  this->name_ = name;
}

}  // namespace amdinfer
