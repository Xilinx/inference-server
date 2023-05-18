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
 * @brief Implements the objects used to hold incoming inference requests
 */

#include "amdinfer/core/model_metadata.hpp"

#include "amdinfer/core/inference_request.hpp"

namespace amdinfer {

ModelMetadata::ModelMetadata(const std::string &name,
                             const std::string &platform) {
  this->name_ = name;
  this->platform_ = platform;
  this->ready_ = false;
}

void ModelMetadata::addInputTensor(const std::string &name,
                                   std::initializer_list<int64_t> shape,
                                   DataType datatype) {
  this->inputs_.emplace_back(name, shape, datatype);
}

void ModelMetadata::addInputTensor(const std::string &name,
                                   std::vector<int> shape, DataType datatype) {
  std::vector<int64_t> new_shape;
  std::copy(shape.begin(), shape.end(), std::back_inserter(new_shape));
  this->inputs_.emplace_back(name, new_shape, datatype);
}

void ModelMetadata::addInputTensor(const Tensor &tensor) {
  this->inputs_.emplace_back(tensor.getName(), tensor.getShape(),
                             tensor.getDatatype());
}

void ModelMetadata::addOutputTensor(const std::string &name,
                                    std::initializer_list<int64_t> shape,
                                    DataType datatype) {
  this->outputs_.emplace_back(name, shape, datatype);
}

void ModelMetadata::addOutputTensor(const std::string &name,
                                    std::vector<int> shape, DataType datatype) {
  std::vector<int64_t> new_shape;
  std::copy(shape.begin(), shape.end(), std::back_inserter(new_shape));
  this->outputs_.emplace_back(name, new_shape, datatype);
}

void ModelMetadata::addOutputTensor(const Tensor &tensor) {
  this->outputs_.emplace_back(tensor.getName(), tensor.getShape(),
                              tensor.getDatatype());
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
