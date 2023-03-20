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

#include "amdinfer/core/tensor.hpp"

#include <cassert>

#include "amdinfer/util/containers.hpp"
#include "amdinfer/util/memory.hpp"

namespace amdinfer {

Tensor::Tensor(std::string name, std::vector<uint64_t> shape,
               DataType data_type)
  : name_(std::move(name)), shape_(std::move(shape)), data_type_(data_type) {}

const std::string &Tensor::getName() const & { return this->name_; }

std::string Tensor::getName() && { return std::move(this->name_); }

void Tensor::setName(std::string name) { name_ = std::move(name); }

const std::vector<uint64_t> &Tensor::getShape() const & { return shape_; }

std::vector<uint64_t> Tensor::getShape() && { return std::move(shape_); }

void Tensor::setShape(std::vector<uint64_t> shape) {
  shape_ = std::move(shape);
}

[[nodiscard]] DataType Tensor::getDatatype() const { return this->data_type_; }

void Tensor::setDatatype(DataType data_type) { data_type_ = data_type; }

size_t Tensor::getSize() const { return util::containerProduct(shape_); }

struct TensorSizes {
  size_t name;
  size_t shape;
  size_t data_type;
};

size_t Tensor::serializeSize() const {
  auto size = sizeof(TensorSizes);
  size += name_.length();
  size += shape_.size() * sizeof(uint64_t);
  size += sizeof(uint8_t);
  return size;
}

std::byte *Tensor::serialize(std::byte *data_out) const {
  auto *data = data_out;
  TensorSizes metadata{name_.length(), shape_.size() * sizeof(uint64_t),
                       sizeof(uint8_t)};
  data = util::copy(metadata, data, sizeof(TensorSizes));
  data = util::copy(name_.c_str(), data, metadata.name);
  data = util::copy(shape_.data(), data, metadata.shape);
  data = util::copy(static_cast<uint8_t>(data_type_), data, metadata.data_type);
  assert(data_out + Tensor::serializeSize() == data);
  return data;
}

const std::byte *Tensor::deserialize(const std::byte *data_in) {
  const auto metadata = *reinterpret_cast<const TensorSizes *>(data_in);
  data_in += sizeof(TensorSizes);

  name_.resize(metadata.name);
  std::memcpy(name_.data(), data_in, metadata.name);
  data_in += metadata.name;

  shape_.resize(metadata.shape / sizeof(uint64_t));
  std::memcpy(shape_.data(), data_in, metadata.shape);
  data_in += metadata.shape;

  uint8_t type = 0;
  std::memcpy(&type, data_in, metadata.data_type);
  data_type_ = static_cast<DataType::Value>(type);
  data_in += metadata.data_type;
  return data_in;
}

}  // namespace amdinfer
