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

#ifndef GUARD_AMDINFER_CORE_INFERENCE_REQUEST
#define GUARD_AMDINFER_CORE_INFERENCE_REQUEST

#include <string>
#include <vector>

#include "amdinfer/core/data_types.hpp"
#include "amdinfer/core/inference_tensor.hpp"
#include "amdinfer/core/parameters.hpp"
#include "amdinfer/declarations.hpp"

namespace amdinfer {

/**
 * @brief Holds an inference request's input data
 */
class InferenceRequestInput : public InferenceTensor {
 public:
  /// Constructs a new InferenceRequestInput object
  InferenceRequestInput();

  /**
   * @brief Construct a new InferenceRequestInput object
   *
   * @param data pointer to data
   * @param shape shape of the data
   * @param data_type type of the data
   * @param name name to assign
   */
  InferenceRequestInput(void *data, std::vector<uint64_t> shape,
                        DataType data_type, std::string name = "");

  /// Set the request's data
  void setData(void *buffer);
  /// Get a pointer to the request's data
  [[nodiscard]] void *getData() const;

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
   * @return std::byte* updated address
   */
  std::byte *serialize(std::byte *data_out) const override;
  /**
   * @brief Deserializes the data at the provided memory address to initialize
   * this object. If the memory cannot be deserialized, an exception is thrown.
   *
   * @param data_in a pointer to the serialized data for this object type
   * @return std::byte* updated address
   */
  const std::byte *deserialize(const std::byte *data_in) override;

  /// Provides an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os,
                                  InferenceRequestInput const &my_class);

 private:
  void *data_ = nullptr;
};

/**
 * @brief Holds an inference request's output data
 *
 */
class InferenceRequestOutput {
 public:
  /// Constructs a new Request Output object
  InferenceRequestOutput();

  /// Sets the request's data
  void setData(void *buffer) { this->data_ = buffer; }

  /// Takes the request's data
  void *getData() { return this->data_; }

  /// Gets the output tensor's name
  [[nodiscard]] std::string getName() const { return this->name_; }
  /// Set the output tensor's name
  void setName(const std::string &name);

  /**
   * @brief Sets the output tensor's parameters
   *
   * @param parameters pointer to parameters to assign
   */
  void setParameters(ParameterMap parameters) {
    parameters_ = std::move(parameters);
  }
  /// @brief Gets the output tensor's parameters
  const ParameterMap &getParameters() const & { return parameters_; }
  /// @brief Gets the output tensor's parameters
  ParameterMap getParameters() && { return std::move(parameters_); }

 private:
  std::string name_;
  ParameterMap parameters_;
  void *data_;
};

/**
 * @brief Creates an inference request object based on KServe's V2 spec that
 * is used to communicate between workers
 */
class InferenceRequest {
 public:
  // Construct a new InferenceRequest object
  InferenceRequest() = default;

  /**
   * @brief Sets the request's callback function used by the last worker to
   * respond back to the client
   *
   * @param callback a function pointer that accepts a InferenceResponse object
   */
  void setCallback(Callback &&callback);
  /**
   * @brief Get the request's callback function used by the last worker to
   * respond back to the client
   */
  Callback getCallback();
  /**
   * @brief Runs the request's callback function.
   *
   * @param response the response data
   */
  void runCallback(const InferenceResponse &response);
  /**
   * @brief Runs the request's callback function and clear it after. This
   * prevents calling the callback multiple times. If this function is called
   * again, it's a no-op.
   *
   * @param response the response data
   */
  void runCallbackOnce(const InferenceResponse &response);
  /**
   * @brief Runs the request's callback function with an error response. The
   * callback function is not cleared
   *
   * @param error_msg error message to send back to the client
   */
  void runCallbackError(std::string_view error_msg);

  /**
   * @brief Constructs and adds a new input tensor to this request
   *
   * @param data pointer to data to add
   * @param shape shape of the data
   * @param data_type the datatype of the data
   * @param name the name of the input tensor
   */
  void addInputTensor(void *data, const std::vector<uint64_t> &shape,
                      DataType data_type, const std::string &name = "");

  /**
   * @brief Adds a new input tensor to this request
   *
   * @param input an existing InferenceRequestInput object
   */
  void addInputTensor(InferenceRequestInput input);

  /**
   * @brief Set the data pointer for an input tensor, if it exists
   *
   * @param index index for the input tensor
   * @param data pointer to assign to its data member
   */
  void setInputTensorData(size_t index, void *data);
  /**
   * @brief Adds a new output tensor to this request
   *
   * @param output an existing InferenceRequestOutput object
   */
  void addOutputTensor(const InferenceRequestOutput &output);

  /// Gets a vector of all the input request objects
  [[nodiscard]] const std::vector<InferenceRequestInput> &getInputs() const;
  /// Get the number of input request objects
  [[nodiscard]] size_t getInputSize() const;

  /// Gets a vector of the requested output information
  [[nodiscard]] const std::vector<InferenceRequestOutput> &getOutputs() const;

  /**
   * @brief Gets the ID associated with this request
   *
   * @return std::string
   */
  [[nodiscard]] const std::string &getID() const { return id_; }
  /**
   * @brief Sets the ID associated with this request
   *
   * @param id ID to set
   */
  void setID(std::string_view id) { id_ = id; }

  /// Get the request's parameters
  [[nodiscard]] const ParameterMap &getParameters() const & {
    return parameters_;
  }
  /// Get the request's parameters
  [[nodiscard]] ParameterMap getParameters() && {
    return std::move(parameters_);
  }
  /**
   * @brief Sets the parameters for the request
   *
   * @param parameters pointer to the parameters
   */
  void setParameters(ParameterMap parameters) {
    parameters_ = std::move(parameters);
  }

 private:
  std::string id_;
  ParameterMap parameters_;
  std::vector<InferenceRequestInput> inputs_;
  std::vector<InferenceRequestOutput> outputs_;
  Callback callback_;

  // TODO(varunsh): do we need this still?
  friend class FakeInferenceRequest;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_INFERENCE_REQUEST
