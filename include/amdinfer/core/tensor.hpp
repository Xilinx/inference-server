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

#ifndef GUARD_AMDINFER_CORE_TENSOR
#define GUARD_AMDINFER_CORE_TENSOR

#include <string>
#include <vector>

#include "amdinfer/core/data_types.hpp"
#include "amdinfer/core/mixins.hpp"

namespace amdinfer {

/**
 * @brief Describe a tensor with a name, shape and datatype
 */
class Tensor : public Serializable {
 public:
  Tensor(std::string name, std::vector<uint64_t> shape, DataType data_type);

  virtual ~Tensor() = default;

  /// Get the tensor's name
  [[nodiscard]] const std::string &getName() const &;
  [[nodiscard]] std::string getName() &&;
  /// Set the tensor's name
  void setName(std::string name);

  /// Get the tensor's shape
  [[nodiscard]] const std::vector<uint64_t> &getShape() const &;
  [[nodiscard]] std::vector<uint64_t> getShape() &&;
  /// Sets the tensor's shape
  void setShape(std::vector<uint64_t> shape);

  /// Get the tensor's datatype
  [[nodiscard]] DataType getDatatype() const;
  /// Set the tensor's data type
  void setDatatype(DataType data_type);

  /// Get the tensor's size (number of elements)
  [[nodiscard]] size_t getSize() const;

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
  std::string name_;
  std::vector<uint64_t> shape_;
  DataType data_type_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_TENSOR
