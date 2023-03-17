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

#ifndef GUARD_AMDINFER_CORE_INFERENCE_TENSOR
#define GUARD_AMDINFER_CORE_INFERENCE_TENSOR

#include "amdinfer/core/parameters.hpp"  // for ParameterMap
#include "amdinfer/core/tensor.hpp"      // IWYU pragma: export

namespace amdinfer {

class InferenceTensor : public Tensor {
 public:
  InferenceTensor(std::string name, std::vector<uint64_t> shape,
                  DataType data_type);

  /// Get the tensor's parameters
  [[nodiscard]] const ParameterMap &getParameters() const &;
  [[nodiscard]] ParameterMap getParameters() &&;

  /**
   * @brief Set the tensor's parameters
   *
   * @param parameters
   */
  void setParameters(ParameterMap parameters);

  /**
   * @brief Returns the size of the serialized data
   *
   * @return size_t
   */
  [[nodiscard]] size_t serializeSize() const override;
  /**
   * @brief Serializes the object to the provided memory address. There should
   * be sufficient space to store the serialized object.
   *
   * @param data_out
   */
  std::byte *serialize(std::byte *data_out) const override;
  /**
   * @brief Deserializes the data at the provided memory address to initialize
   * this object. If the memory cannot be deserialized, an exception is thrown.
   *
   * @param data_in a pointer to the serialized data for this object type
   */
  const std::byte *deserialize(const std::byte *data_in) override;

 private:
  ParameterMap parameters_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_INFERENCE_TENSOR
