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

#ifndef GUARD_AMDINFER_CORE_MODEL_METADATA
#define GUARD_AMDINFER_CORE_MODEL_METADATA

#include <string>
#include <vector>

#include "amdinfer/core/data_types.hpp"
#include "amdinfer/core/tensor.hpp"

namespace amdinfer {

class InferenceRequestInput;

using ModelMetadataTensor = Tensor;

/**
 * @brief This class holds the metadata associated with a model (per the KServe
 * spec). This allows clients to query this information from the server.
 *
 */
class ModelMetadata {
 public:
  /**
   * @brief Constructs a new Model Metadata object
   *
   * @param name Name of the model
   * @param platform the platform this model runs on
   */
  ModelMetadata(const std::string &name, const std::string &platform);

  /**
   * @brief Adds an input tensor to this model
   *
   * @param name name of the tensor
   * @param shape shape of the tensor
   * @param datatype datatype of the tensor
   */
  void addInputTensor(const std::string &name, std::vector<int64_t> shape,
                      DataType datatype);
  void addInputTensor(const std::string &name,
                      std::initializer_list<int64_t> shape, DataType datatype);
  /**
   * @brief Adds an input tensor to this model
   *
   * @param name name of the tensor
   * @param shape shape of the tensor
   * @param datatype datatype of the tensor
   */
  void addInputTensor(const std::string &name, std::vector<int> shape,
                      DataType datatype);
  /**
   * @brief Adds an input tensor to this model
   *
   * @param tensor
   */
  void addInputTensor(const Tensor &tensor);

  /**
   * @brief Gets the input tensor' metadata for this model
   *
   * @return const std::vector<ModelMetadataTensor>&
   */
  [[nodiscard]] const std::vector<ModelMetadataTensor> &getInputs() const;

  /**
   * @brief Adds an output tensor to this model
   *
   * @param name name of the tensor
   * @param shape shape of the tensor
   * @param datatype datatype of the tensor
   */
  void addOutputTensor(const std::string &name, std::vector<int64_t> shape,
                       DataType datatype);
  void addOutputTensor(const std::string &name,
                       std::initializer_list<int64_t> shape, DataType datatype);
  /**
   * @brief Adds an output tensor to this model
   *
   * @param name name of the tensor
   * @param shape shape of the tensor
   * @param datatype datatype of the tensor
   */
  void addOutputTensor(const std::string &name, std::vector<int> shape,
                       DataType datatype);
  /**
   * @brief Adds an output tensor to this model
   *
   * @param tensor
   */
  void addOutputTensor(const Tensor &tensor);

  /// @brief Gets the output tensors' metadata for this model
  [[nodiscard]] const std::vector<ModelMetadataTensor> &getOutputs() const;

  /// Sets the model's name
  void setName(const std::string &name);
  /// Gets the model's name
  [[nodiscard]] const std::string &getName() const;

  [[nodiscard]] const std::string &getPlatform() const;

  /// Marks this model as ready/not ready
  void setReady(bool ready);
  /// Checks if this model is ready
  [[nodiscard]] bool isReady() const;

 private:
  std::string name_;
  std::vector<std::string> versions_;
  std::string platform_;
  std::vector<ModelMetadataTensor> inputs_;
  std::vector<ModelMetadataTensor> outputs_;
  bool ready_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MODEL_METADATA
