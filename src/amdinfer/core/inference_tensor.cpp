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

#include "amdinfer/core/inference_tensor.hpp"

#include "amdinfer/util/memory.hpp"

namespace amdinfer {

InferenceTensor::InferenceTensor(std::string name, std::vector<uint64_t> shape,
                                 DataType data_type)
  : Tensor(std::move(name), std::move(shape), data_type) {}

const ParameterMap& InferenceTensor::getParameters() const& {
  return this->parameters_;
}

InferenceTensor::InferenceTensor(const Tensor& tensor) : Tensor(tensor) {}

ParameterMap InferenceTensor::getParameters() && {
  return std::move(this->parameters_);
}

void InferenceTensor::setParameters(ParameterMap parameters) {
  parameters_ = std::move(parameters);
}

size_t InferenceTensor::serializeSize() const {
  auto size = Tensor::serializeSize();
  size += parameters_.serializeSize();
  return size;
}

std::byte* InferenceTensor::serialize(std::byte* data_out) const {
  auto* data = data_out;
  data = Tensor::serialize(data);
  data = parameters_.serialize(data);
  assert(data_out + InferenceTensor::serializeSize() == data);
  return data;
}

const std::byte* InferenceTensor::deserialize(const std::byte* data_in) {
  data_in = Tensor::deserialize(data_in);
  data_in = parameters_.deserialize(data_in);
  return data_in;
}

}  // namespace amdinfer
