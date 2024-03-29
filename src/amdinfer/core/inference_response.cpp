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

#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse

#include "amdinfer/core/inference_request.hpp"  // for InferenceRequest
#include "amdinfer/util/memory.hpp"

namespace amdinfer {

InferenceResponse::InferenceResponse() = default;

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

#ifdef AMDINFER_ENABLE_TRACING
void InferenceResponse::setContext(StringMap &&context) {
  this->context_ = std::move(context);
}

const StringMap &InferenceResponse::getContext() const {
  return this->context_;
}
#endif

std::ostream &operator<<(std::ostream &os, InferenceResponse const &self) {
  os << "Inference Response:\n";
  os << "  Model: " << self.model_ << "\n";
  os << "  ID: " << self.id_ << "\n";
  os << "  Parameters:\n";
  os << "    " << *(self.parameters_.get()) << "\n";
  os << "  Outputs:\n";
  for (const auto &output : self.outputs_) {
    os << "    " << output << "\n";
  }
  os << "  Error Message: " << self.error_msg_ << "\n";
  return os;
}

InferenceResponseOutput::InferenceResponseOutput()
  : InferenceTensor("", {}, DataType::Unknown) {}

void InferenceResponseOutput::setData(std::vector<std::byte> &&buffer) {
  data_ = std::move(buffer);
}

void *InferenceResponseOutput::getData() const {
  return (void *)data_.data();  // NOLINT(google-readability-casting)
}

struct InferenceResponseOutputSizes {
  size_t data;
};

size_t InferenceResponseOutput::serializeSize() const {
  auto size = InferenceTensor::serializeSize();
  size += sizeof(InferenceResponseOutputSizes);
  size += data_.size();
  return size;
}

std::byte *InferenceResponseOutput::serialize(std::byte *data_out) const {
  auto *data = data_out;
  data = InferenceTensor::serialize(data);

  InferenceResponseOutputSizes metadata{data_.size()};
  data = util::copy(metadata, data, sizeof(InferenceResponseOutputSizes));
  data = util::copy(data_.data(), data, metadata.data);
  assert(data_out + this->serializeSize() == data);
  return data;
}

const std::byte *InferenceResponseOutput::deserialize(
  const std::byte *data_in) {
  data_in = InferenceTensor::deserialize(data_in);

  const auto metadata =
    *reinterpret_cast<const InferenceResponseOutputSizes *>(data_in);
  data_in += sizeof(InferenceResponseOutputSizes);

  data_.resize(metadata.data);
  return util::copy(data_in, data_.data(), metadata.data);
}

std::ostream &operator<<(std::ostream &os,
                         InferenceResponseOutput const &self) {
  os << "InferenceResponseOutput:\n";
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

}  // namespace amdinfer
