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
 * @brief Implements the objects used to hold incoming inference requests
 */

#include "amdinfer/core/predict_api.hpp"

#include <algorithm>  // for copy
#include <cassert>    // for assert
#include <cstring>    // for memcpy
#include <iterator>   // for back_insert_iterator, back_inse...
#include <numeric>    // for accumulate
#include <utility>    // for pair, make_pair, move

#include "amdinfer/build_options.hpp"  // for AMDINFER_ENABLE_TRACING

namespace amdinfer {

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

size_t RequestParameters::serializeSize() const {
  // 1 for num of parameters plus 3 for each parameter for type index, key size
  // and value size
  auto size = ((this->size() * 3) + 1) * sizeof(size_t);
  for (const auto &[key, value] : parameters_) {
    size += key.size();
    std::visit(
      [&size](const auto &param) {
        using T = std::decay_t<decltype(param)>;
        if constexpr (std::is_same_v<T, std::string>) {
          size += param.size();
        } else {
          size += sizeof(param);
        }
      },
      value);
  }
  return size;
}

template <typename T>
std::byte *copy(const T &src, std::byte *dst, size_t count) {
  if constexpr (std::is_pointer_v<T>) {
    std::memcpy(dst, src, count);
  } else {
    std::memcpy(dst, &src, count);
  }
  return dst + count;
}

void RequestParameters::serialize(std::byte *data_out) const {
  auto size = this->size();
  std::string foo;
  data_out = copy(size, data_out, sizeof(size_t));
  for (const auto &[key, value] : parameters_) {
    data_out = copy(value.index(), data_out, sizeof(size_t));
    data_out = copy(key.size(), data_out, sizeof(size_t));
    std::visit(
      [&](const auto &param) {
        using T = std::decay_t<decltype(param)>;
        if constexpr (std::is_same_v<T, std::string>) {
          data_out = copy(param.size(), data_out, sizeof(size_t));
        } else {
          data_out = copy(sizeof(param), data_out, sizeof(size_t));
        }
      },
      value);
  }
  for (const auto &[key, value] : parameters_) {
    data_out = copy(key.c_str(), data_out, key.size());
    std::visit(
      [&](const auto &param) {
        using T = std::decay_t<decltype(param)>;
        if constexpr (std::is_same_v<T, std::string>) {
          data_out = copy(param.c_str(), data_out, param.size());
        } else {
          data_out = copy(param, data_out, sizeof(param));
        }
      },
      value);
  }
}

/**
 * @brief This is used to turn a runtime index into a variant with the right
 * type
 *
 * @tparam Ts: the types of the variant. Order matters!
 * @param i index of the variant type to create
 * @return std::variant<Ts...>
 *
 * https://www.reddit.com/r/cpp/comments/f8cbzs/comment/fimjm2f/?context=3
 */
template <typename... Ts>
[[nodiscard]] std::variant<Ts...> expand_type(std::size_t i) {
  assert(i < sizeof...(Ts));
  static constexpr auto table =
    std::array{+[]() { return std::variant<Ts...>{Ts{}}; }...};
  return table[i]();
}

void RequestParameters::deserialize(const std::byte *data_in) {
  parameters_.clear();

  auto size = std::to_integer<size_t>(*data_in);
  data_in += sizeof(size_t);
  std::vector<std::tuple<size_t, size_t, size_t>> params;
  params.reserve(size);
  for (auto i = 0U; i < size; i++) {
    auto index = std::to_integer<size_t>(*data_in);
    data_in += sizeof(size_t);
    auto key_size = std::to_integer<size_t>(*data_in);
    data_in += sizeof(size_t);
    auto data_size = std::to_integer<size_t>(*data_in);
    data_in += sizeof(size_t);
    params.emplace_back(index, key_size, data_size);
  }
  for (const auto &[index, key_size, data_size] : params) {
    std::string key;
    key.resize(key_size);
    std::memcpy(key.data(), data_in, key_size);
    data_in += key_size;
    Parameter param = expand_type<bool, int32_t, double, std::string>(index);
    const auto &n = data_size;
    std::visit(
      [&](auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::string>) {
          value.resize(n);
          std::memcpy(value.data(), data_in, n);
          data_in += n;
        } else {
          std::memcpy(&value, data_in, n);
          data_in += n;
        }
      },
      param);
    parameters_.try_emplace(key, param);
  }
}

void InferenceRequest::setCallback(Callback &&callback) {
  callback_ = std::move(callback);
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

void InferenceRequest::addInputTensor(void *data,
                                      const std::vector<uint64_t> &shape,
                                      DataType dataType,
                                      const std::string &name) {
  this->inputs_.emplace_back(data, shape, dataType, name);
}

void InferenceRequest::addInputTensor(InferenceRequestInput input) {
  this->inputs_.push_back(std::move(input));
}

const std::vector<InferenceRequestInput> &InferenceRequest::getInputs() const {
  return this->inputs_;
}

size_t InferenceRequest::getInputSize() { return this->inputs_.size(); }

const std::vector<InferenceRequestOutput> &InferenceRequest::getOutputs()
  const {
  return this->outputs_;
}

void InferenceRequest::addOutputTensor(const InferenceRequestOutput &output) {
  this->outputs_.push_back(output);
}

InferenceRequestInput::InferenceRequestInput(void *data,
                                             std::vector<uint64_t> shape,
                                             DataType dataType,
                                             std::string name)
  : dataType_(dataType) {
  this->data_ = data;
  this->shape_ = std::move(shape);
  this->name_ = std::move(name);
  this->parameters_ = std::make_unique<RequestParameters>();
}

InferenceRequestInput::InferenceRequestInput() : dataType_(DataType::Uint32) {
  this->data_ = nullptr;
  this->name_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
}

void InferenceRequestInput::setName(std::string name) {
  this->name_ = std::move(name);
}

void InferenceRequestInput::setDatatype(DataType type) {
  this->dataType_ = type;
}

void InferenceRequestInput::setData(void *buffer) { this->data_ = buffer; }

void InferenceRequestInput::setData(std::vector<std::byte> &&buffer) {
  this->shared_data_ = std::move(buffer);
}

bool InferenceRequestInput::sharedData() const {
  return this->shared_data_.empty();
}

size_t InferenceRequestInput::getSize() const {
  return std::accumulate(this->shape_.begin(), this->shape_.end(), 1,
                         std::multiplies<>());
}

void *InferenceRequestInput::getData() const {
  if (!this->shared_data_.empty()) {
    return (void *)shared_data_.data();
  }
  return this->data_;
}

struct InferenceRequestInputSizes {
  size_t name;
  size_t shape;
  size_t dataType;
  size_t parameters;
  size_t data;
  size_t shared_data;
};

size_t InferenceRequestInput::serializeSize() const {
  auto size = sizeof(InferenceRequestInputSizes);
  size += name_.length();
  size += shape_.size() * sizeof(uint64_t);
  size += sizeof(uint8_t);
  size += parameters_->serializeSize();
  if (!shared_data_.empty()) {
    size += shared_data_.size();
  } else {
    size += sizeof(data_);
  }
  return size;
}

void InferenceRequestInput::serialize(std::byte *data_out) const {
  InferenceRequestInputSizes metadata{name_.length(),
                                      shape_.size() * sizeof(uint64_t),
                                      sizeof(uint8_t),
                                      parameters_->serializeSize(),
                                      0,
                                      0};
  if (!shared_data_.empty()) {
    metadata.shared_data = this->getSize() * dataType_.size();
  } else {
    metadata.data = sizeof(data_);
  }
  data_out = copy(metadata, data_out, sizeof(InferenceRequestInputSizes));
  data_out = copy(name_.c_str(), data_out, metadata.name);
  data_out = copy(shape_.data(), data_out, metadata.shape);
  data_out = copy(static_cast<uint8_t>(dataType_), data_out, metadata.dataType);
  parameters_->serialize(data_out);
  data_out += metadata.parameters;
  if (!shared_data_.empty()) {
    amdinfer::copy(shared_data_.data(), data_out, metadata.shared_data);
  } else {
    std::memcpy(data_out, &data_, metadata.data);
  }
}

void InferenceRequestInput::deserialize(const std::byte *data_in) {
  const auto metadata =
    *reinterpret_cast<const InferenceRequestInputSizes *>(data_in);
  data_in += sizeof(InferenceRequestInputSizes);

  name_.resize(metadata.name);
  std::memcpy(name_.data(), data_in, metadata.name);
  data_in += metadata.name;

  shape_.resize(metadata.shape / sizeof(uint64_t));
  std::memcpy(shape_.data(), data_in, metadata.shape);
  data_in += metadata.shape;

  uint8_t type;
  std::memcpy(&type, data_in, metadata.dataType);
  data_in += metadata.dataType;
  dataType_ = static_cast<DataType::Value>(type);

  parameters_ = std::make_shared<RequestParameters>();
  parameters_->deserialize(data_in);
  data_in += parameters_->serializeSize();

  if (metadata.shared_data != 0) {
    shared_data_.resize(metadata.shared_data);
    std::memcpy(shared_data_.data(), data_in, metadata.shared_data);
  } else {
    std::memcpy(&data_, data_in, metadata.data);
  }
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

#ifdef AMDINFER_ENABLE_TRACING
void InferenceResponse::setContext(StringMap &&context) {
  this->context_ = std::move(context);
}

const StringMap &InferenceResponse::getContext() const {
  return this->context_;
}
#endif

ModelMetadataTensor::ModelMetadataTensor(const std::string &name,
                                         DataType datatype,
                                         std::vector<uint64_t> shape)
  : datatype_(datatype) {
  this->name_ = name;
  this->shape_ = std::move(shape);
}

const std::string &ModelMetadataTensor::getName() const { return this->name_; }

const DataType &ModelMetadataTensor::getDataType() const {
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

void ModelMetadata::addInputTensor(const std::string &name, DataType datatype,
                                   std::initializer_list<uint64_t> shape) {
  this->inputs_.emplace_back(name, datatype, shape);
}

void ModelMetadata::addInputTensor(const std::string &name, DataType datatype,
                                   std::vector<int> shape) {
  std::vector<uint64_t> new_shape;
  std::copy(shape.begin(), shape.end(), std::back_inserter(new_shape));
  this->inputs_.emplace_back(name, datatype, new_shape);
}

void ModelMetadata::addOutputTensor(const std::string &name, DataType datatype,
                                    std::initializer_list<uint64_t> shape) {
  this->outputs_.emplace_back(name, datatype, shape);
}

void ModelMetadata::addOutputTensor(const std::string &name, DataType datatype,
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

}  // namespace amdinfer
