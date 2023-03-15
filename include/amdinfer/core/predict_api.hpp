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
 * @brief Defines the objects used to hold inference requests and responses
 */

#ifndef GUARD_AMDINFER_CORE_PREDICT_API
#define GUARD_AMDINFER_CORE_PREDICT_API

#include <cstddef>           // for size_t, byte
#include <cstdint>           // for uint64_t, int32_t
#include <functional>        // for function, less
#include <future>            // for promise
#include <initializer_list>  // for initializer_list
#include <map>               // for map, operator==, map<>::...
#include <memory>            // for shared_ptr, allocator
#include <sstream>           // for operator<<, ostream, bas...
#include <string>            // for string, operator<<, char...
#include <string_view>       // for string_view
#include <unordered_set>     // for unordered_set
#include <utility>           // for move
#include <variant>           // for operator!=, operator<
#include <vector>            // for vector

#include "amdinfer/build_options.hpp"    // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"  // for DataType, mapTypeToStr
#include "amdinfer/core/mixins.hpp"      // for Serializable
#include "amdinfer/core/parameters.hpp"  // for ParameterMap
#include "amdinfer/declarations.hpp"     // for InferenceResponseOutput

namespace amdinfer {

struct ServerMetadata {
  /// Name of the server
  std::string name;
  /// Version of the server
  std::string version;
  /**
   * @brief The extensions supported by the server. The KServe specification
   * allows servers to support custom extensions and return them with a
   * metadata request.
   */
  std::unordered_set<std::string> extensions;
};

/**
 * @brief Holds an inference request's input data
 */
class InferenceRequestInput : public Serializable {
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

  /// Sets the request's data
  void setData(void *buffer);
  /// Sets the request's shared data
  void setData(std::vector<std::byte> &&buffer);
  /// Checks if the stored data is shared
  [[nodiscard]] bool sharedData() const;

  /// Gets a pointer to the request's data
  [[nodiscard]] void *getData() const;

  /// Gets the input tensor's name
  [[nodiscard]] const std::string &getName() const { return this->name_; }
  /// Sets the input tensor's name
  void setName(std::string name);

  /// Gets the input tensor's shape
  [[nodiscard]] const std::vector<uint64_t> &getShape() const {
    return this->shape_;
  }
  /// Sets the tensor's shape
  void setShape(std::initializer_list<uint64_t> shape) { this->shape_ = shape; }
  /// Sets the tensor's shape
  void setShape(const std::vector<uint64_t> &shape) { this->shape_ = shape; }
  /// Sets the tensor's shape
  void setShape(const std::vector<int32_t> &shape) {
    this->shape_.reserve(shape.size());
    for (const auto &index : shape) {
      this->shape_.push_back(index);
    }
  }

  /// Gets the input tensor's datatype
  [[nodiscard]] DataType getDatatype() const { return this->data_type_; }
  /// Set the tensor's data type
  void setDatatype(DataType type);

  /// Gets the input tensor's parameters
  [[nodiscard]] const ParameterMap &getParameters() const & {
    return this->parameters_;
  }

  [[nodiscard]] ParameterMap getParameters() && {
    return std::move(this->parameters_);
  }

  /**
   * @brief Sets the input tensor's parameters
   *
   * @param parameters pointer to parameters to assign
   */
  void setParameters(ParameterMap parameters) {
    parameters_ = std::move(parameters);
  }

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
  void serialize(std::byte *data_out) const override;
  /**
   * @brief Deserializes the data at the provided memory address to initialize
   * this object. If the memory cannot be deserialized, an exception is thrown.
   *
   * @param data_in a pointer to the serialized data for this object type
   */
  void deserialize(const std::byte *data_in) override;

  /// Provides an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os,
                                  InferenceRequestInput const &my_class) {
    os << "InferenceRequestInput:\n";
    os << "  Name: " << my_class.name_ << "\n";
    os << "  Shape: ";
    for (const auto &index : my_class.shape_) {
      os << index << ",";
    }
    os << "\n";
    os << "  Datatype: " << my_class.data_type_.str() << "\n";
    os << "  Parameters:\n";
    os << my_class.parameters_ << "\n";
    os << "  Data: " << my_class.getData() << "\n";
    return os;
  }

 private:
  std::string name_;
  std::vector<uint64_t> shape_;
  DataType data_type_;
  ParameterMap parameters_;
  void *data_;
  std::vector<std::byte> shared_data_;
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
 * @brief Creates an inference response object based on KServe's V2 spec that
 * is used to respond back to clients.
 */
class InferenceResponse {
 public:
  /// Constructs a new InferenceResponse object
  InferenceResponse();

  /// Constructs a new InferenceResponse error object
  explicit InferenceResponse(const std::string &error);

  /// Gets a vector of the requested output information
  [[nodiscard]] std::vector<InferenceResponseOutput> getOutputs() const;
  /**
   * @brief Adds an output tensor to the response
   *
   * @param output an output tensor
   */
  void addOutput(const InferenceResponseOutput &output);

  /// Gets the ID of the response
  std::string getID() const { return id_; }
  /// Sets the ID of the response
  void setID(const std::string &id);
  /// sets the model name of the response
  void setModel(const std::string &model);
  /// gets the model name of the response
  std::string getModel();

  /// Checks if this is an error response
  bool isError() const;
  /// Gets the error message if it exists. Defaults to an empty string
  std::string getError() const;

#ifdef AMDINFER_ENABLE_TRACING
  /**
   * @brief Sets the Context object to propagate tracing data on
   *
   * @param context
   */
  void setContext(StringMap &&context);
  /// Gets the response's context to use to initialize a new span
  const StringMap &getContext() const;
#endif

  /// Gets a pointer to the parameters associated with this response
  ParameterMap *getParameters() { return this->parameters_.get(); }

  /// Provides an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os,
                                  InferenceResponse const &my_class) {
    os << "Inference Response:\n";
    os << "  Model: " << my_class.model_ << "\n";
    os << "  ID: " << my_class.id_ << "\n";
    os << "  Parameters:\n";
    os << "    " << *(my_class.parameters_.get()) << "\n";
    os << "  Outputs:\n";
    for (const auto &output : my_class.outputs_) {
      os << "    " << output << "\n";
    }
    os << "  Error Message: " << my_class.error_msg_ << "\n";
    return os;
  }

 private:
  std::string model_;
  std::string id_;
  std::shared_ptr<ParameterMap> parameters_;
  std::vector<InferenceResponseOutput> outputs_;
  std::string error_msg_;
#ifdef AMDINFER_ENABLE_TRACING
  StringMap context_;
#endif
};

using Callback = std::function<void(const InferenceResponse &)>;

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
using InferenceResponsePromisePtr =
  std::shared_ptr<std::promise<InferenceResponse>>;

/**
 * @brief This class holds the metadata associated with an input tensor
 *
 */
class ModelMetadataTensor final {
 public:
  /**
   * @brief Construct a new Model Metadata Tensor object
   *
   * @param name name of the tensor
   * @param datatype the datatype this tensor accepts
   * @param shape the expected shape of the data
   */
  ModelMetadataTensor(const std::string &name, DataType datatype,
                      std::vector<uint64_t> shape);

  /// @brief Gets the name of the tensor
  [[nodiscard]] const std::string &getName() const;
  /// @brief Gets the datatype that this tensor accepts
  [[nodiscard]] const DataType &getDataType() const;
  /// @brief Gets the expected shape of the data
  [[nodiscard]] const std::vector<uint64_t> &getShape() const;

 private:
  std::string name_;
  DataType datatype_;
  std::vector<uint64_t> shape_;
};

/**
 * @brief This class holds the metadata associated with a model (per the KServe
 * spec). This allows clients to query this information from the server.
 *
 */
class ModelMetadata final {
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
   * @param datatype datatype of the tensor
   * @param shape shape of the tensor
   */
  void addInputTensor(const std::string &name, DataType datatype,
                      std::initializer_list<uint64_t> shape);
  /**
   * @brief Adds an input tensor to this model
   *
   * @param name name of the tensor
   * @param datatype datatype of the tensor
   * @param shape shape of the tensor
   */
  void addInputTensor(const std::string &name, DataType datatype,
                      std::vector<int> shape);

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
   * @param datatype datatype of the tensor
   * @param shape shape of the tensor
   */
  void addOutputTensor(const std::string &name, DataType datatype,
                       std::initializer_list<uint64_t> shape);
  /**
   * @brief Adds an output tensor to this model
   *
   * @param name name of the tensor
   * @param datatype datatype of the tensor
   * @param shape shape of the tensor
   */
  void addOutputTensor(const std::string &name, DataType datatype,
                       std::vector<int> shape);

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

namespace std {
template <>
/**
 * @brief Overload the "less than" operator so we can compare two
 * RequestParameter objects. We need this functionality to store objects of
 * this class in a map. Note, since hashing is not implemented, these objects
 * cannot be stored in an unordered_map.
 *
 */
struct less<amdinfer::ParameterMap> {
  /**
   * @brief Implementation of the comparison of two RequestParameter objects.
   * We compare the size and then check each key is present and finally, compare
   * the key values. The types supported in ParameterMap all support
   * direct comparison with the "less than" operator already.
   *
   * @param lhs the RequestParameter object on the left-hand-side
   * @param rhs the RequestParameter object on the right-hand-side
   * @return bool
   */
  bool operator()(const amdinfer::ParameterMap &lhs,
                  const amdinfer::ParameterMap &rhs) const {
    auto lhs_size = lhs.size();
    auto rhs_size = rhs.size();
    auto lhs_map = lhs.data();
    auto rhs_map = rhs.data();
    if (lhs_size == rhs_size) {
      for (const auto &[key, lhs_value] : lhs_map) {
        if (rhs_map.find(key) == rhs_map.end()) {
          return true;
        }
        const auto &rhs_value = rhs_map.at(key);
        if (lhs_value != rhs_value) {
          return lhs_value < rhs_value;
        }
      }
      return false;
    }
    return lhs_size < rhs_size;
  }
};
}  // namespace std

#endif  // GUARD_AMDINFER_CORE_PREDICT_API
