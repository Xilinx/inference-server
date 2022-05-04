// Copyright 2021 Xilinx Inc.
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

#ifndef GUARD_PROTEUS_CORE_PREDICT_API
#define GUARD_PROTEUS_CORE_PREDICT_API

#include <cstddef>           // for size_t, byte
#include <cstdint>           // for uint64_t, int32_t
#include <functional>        // for function, less
#include <future>            // for promise
#include <initializer_list>  // for initializer_list
#include <map>               // for map, operator==, map<>::...
#include <memory>            // for shared_ptr, allocator
#include <ostream>           // for operator<<, ostream, bas...
#include <set>               // for set
#include <sstream>           // for stringstream
#include <string>            // for string, operator<<, char...
#include <string_view>       // for string_view
#include <utility>           // for move
#include <variant>           // for operator!=, operator<
#include <vector>            // for vector

#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"       // for DataType, mapTypeToStr
#include "proteus/helpers/declarations.hpp"  // for InferenceResponseOutput

namespace proteus {

/// parameters in Proteus may be one of these types
using Parameter = std::variant<bool, int32_t, double, std::string>;

/**
 * @brief Holds any parameters from JSON (defined by KServe spec as one of
 * bool, number or string). We further restrict numbers to be doubles or int32.
 *
 */
class RequestParameters {
 public:
  /**
   * @brief Put in a key-value pair
   *
   * @param key key used to store and retrieve the value
   * @param value value to store
   */
  void put(const std::string &key, bool value);
  /**
   * @brief Put in a key-value pair
   *
   * @param key key used to store and retrieve the value
   * @param value value to store
   */
  void put(const std::string &key, double value);
  /**
   * @brief Put in a key-value pair
   *
   * @param key key used to store and retrieve the value
   * @param value value to store
   */
  void put(const std::string &key, int32_t value);
  /**
   * @brief Put in a key-value pair
   *
   * @param key key used to store and retrieve the value
   * @param value value to store
   */
  void put(const std::string &key, const std::string &value);
  /**
   * @brief Put in a key-value pair
   *
   * @param key key used to store and retrieve the value
   * @param value value to store
   */
  void put(const std::string &key, const char *value);

  /**
   * @brief Get a pointer to the named parameter. Returns nullptr if not found
   * or if a bad type is used.
   *
   * @tparam T type of parameter. Must be (bool|double|int32_t|std::string)
   * @param key parameter to get
   * @return T*
   */
  template <typename T>
  T get(const std::string &key) {
    auto &value = this->parameters_.at(key);
    return std::get<T>(value);
  }

  /**
   * @brief Check if a particular parameter exists
   *
   * @param key name of the parameter to check
   * @return bool
   */
  bool has(const std::string &key);
  /**
   * @brief Remove a parameter
   *
   * @param key name of the parameter to remove
   */
  void erase(const std::string &key);
  /// Get the number of parameters
  [[nodiscard]] size_t size() const;
  /// Check if the parameters are empty
  [[nodiscard]] bool empty() const;
  /// Get the underlying data structure holding the parameters
  [[nodiscard]] std::map<std::string, Parameter> data() const;

  auto begin() { return parameters_.begin(); }
  auto cbegin() const { return parameters_.cbegin(); }

  auto end() { return parameters_.end(); }
  auto cend() const { return parameters_.cend(); }

  /// Provide an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os,
                                  RequestParameters const &self) {
    std::stringstream ss;
    ss << "RequestParameters(" << &self << "):\n";
    for (const auto &[key, value] : self.parameters_) {
      ss << "  " << key << ": ";
      std::visit([&](auto &&c) { ss << c; }, value);
      ss << "\n";
    }
    auto tmp = ss.str();
    tmp.pop_back();  // delete trailing newline
    os << tmp;
    return os;
  }

 private:
  std::map<std::string, Parameter> parameters_;
};

using RequestParametersPtr = std::shared_ptr<RequestParameters>;

struct ServerMetadata {
  std::string name;
  std::string version;
  std::set<std::string> extensions;
};

/**
 * @brief Holds an inference request's input data
 */
class InferenceRequestInput {
 public:
  /// Construct a new InferenceRequestInput object
  InferenceRequestInput();

  /**
   * @brief Construct a new InferenceRequestInput object
   *
   * @param data pointer to data
   * @param shape shape of the data
   * @param dataType type of the data
   * @param name name to assign
   */
  InferenceRequestInput(void *data, std::vector<uint64_t> shape,
                        DataType dataType, std::string name = "");

  /// Set the request's data
  void setData(void *buffer) { this->data_ = buffer; }
  /// Set the request's shared data
  void setData(std::shared_ptr<std::byte> buffer) {
    this->shared_data_ = std::move(buffer);
  }

  /// Get a pointer to the request's data
  [[nodiscard]] void *getData() const;

  /// Get the input tensor's name
  const std::string &getName() const { return this->name_; }
  /// Set the input tensor's name
  void setName(std::string name);

  /// Get the input tensor's shape
  const std::vector<uint64_t> &getShape() const { return this->shape_; }
  /// Set the tensor's shape
  void setShape(std::initializer_list<uint64_t> shape) { this->shape_ = shape; }
  /// Set the tensor's shape
  void setShape(const std::vector<uint64_t> &shape) { this->shape_ = shape; }
  /// Set the tensor's shape
  void setShape(const std::vector<int32_t> &shape) {
    this->shape_.reserve(shape.size());
    for (const auto &index : shape) {
      this->shape_.push_back(index);
    }
  }

  /// Get the input tensor's datatype
  DataType getDatatype() const { return this->dataType_; }
  /// Set the tensor's data type
  void setDatatype(DataType type);

  /// Get the input tensor's parameters
  RequestParameters *getParameters() const { return this->parameters_.get(); }
  void setParameters(RequestParametersPtr parameters) {
    parameters_ = parameters;
  }

  /// Set the tensor's size
  size_t getSize() const;

  /// Provide an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os,
                                  InferenceRequestInput const &my_class) {
    os << "InferenceRequestInput:\n";
    os << "  Name: " << my_class.name_ << "\n";
    os << "  Shape: ";
    for (const auto &index : my_class.shape_) {
      os << index << ",";
    }
    os << "\n";
    os << "  Datatype: " << my_class.dataType_.str() << "\n";
    os << "  Parameters:\n";
    if (my_class.parameters_ != nullptr) {
      os << *(my_class.parameters_.get()) << "\n";
    }
    os << "  Data: " << my_class.getData() << "\n";
    return os;
  }

 private:
  std::string name_;
  std::vector<uint64_t> shape_;
  DataType dataType_;
  RequestParametersPtr parameters_;
  void *data_;
  std::shared_ptr<std::byte> shared_data_;

  template <typename U>
  friend class InferenceRequestInputBuilder;
};

/**
 * @brief Holds an inference request's output data
 *
 */
class InferenceRequestOutput {
 public:
  /// Construct a new Request Output object
  InferenceRequestOutput();

  /// Set the request's data
  void setData(void *buffer) { this->data_ = buffer; }

  /// Take the request's data
  void *getData() { return this->data_; }

  /// Get the output tensor's name
  std::string getName() { return this->name_; }
  void setName(const std::string &name);

  void setParameters(RequestParametersPtr parameters) {
    parameters_ = parameters;
  }
  RequestParameters *getParameters() { return parameters_.get(); }

 private:
  std::string name_;
  RequestParametersPtr parameters_;
  void *data_;

  template <typename U>
  friend class InferenceRequestOutputBuilder;
};

/**
 * @brief Creates an inference response object based on KServe's V2 spec that
 * is used to respond back to clients.
 */
class InferenceResponse {
 public:
  /// Construct a new InferenceResponse object
  InferenceResponse();

  /// Construct a new InferenceResponse error object
  explicit InferenceResponse(const std::string &error);

  /// Get a vector of the requested output information
  [[nodiscard]] std::vector<InferenceResponseOutput> getOutputs() const;
  /**
   * @brief Add an output tensor to the response
   *
   * @param output an output tensor
   */
  void addOutput(const InferenceResponseOutput &output);

  /// Get the ID of the response
  std::string getID() { return id_; }
  /// Set the ID of the response
  void setID(const std::string &id);
  /// set the model name of the response
  void setModel(const std::string &model);
  /// get the model name of the response
  std::string getModel();

  /// Check if this is an error response
  bool isError() const;
  /// Get the error message if it exists. Defaults to an empty string
  std::string getError() const;

#ifdef PROTEUS_ENABLE_TRACING
  /**
   * @brief Set the Context object to propagate tracing data on
   *
   * @param context
   */
  void setContext(StringMap &&context);
  /// Get the response's context to use to initialize a new span
  const StringMap &getContext() const;
#endif

  /// Get a pointer to the parameters associated with this response
  RequestParameters *getParameters() { return this->parameters_.get(); }

  /// Provide an implementation to print the class with std::cout to an ostream
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
  std::shared_ptr<RequestParameters> parameters_;
  std::vector<InferenceResponseOutput> outputs_;
  std::string error_msg_;
#ifdef PROTEUS_ENABLE_TRACING
  StringMap context_;
#endif
};

using Callback = std::function<void(const InferenceResponse &)>;

/**
 * @brief Creates an inference request object based on KServe's V2 spec that
 * is used to communicate between workers in Proteus
 */
class InferenceRequest {
 public:
  // Construct a new InferenceRequest object
  InferenceRequest() = default;

  /**
   * @brief Set the request's callback function used by the last worker to
   * respond back to the client
   *
   * @param callback a function pointer that accepts a InferenceResponse object
   */
  void setCallback(Callback &&callback);
  /**
   * @brief Run the request's callback function.
   *
   * @param response the response data
   */
  void runCallback(const InferenceResponse &response);
  /**
   * @brief Run the request's callback function and clear it after. This
   * prevents calling the callback multiple times. If this function is called
   * again, it's a no-op.
   *
   * @param response the response data
   */
  void runCallbackOnce(const InferenceResponse &response);
  /**
   * @brief Run the request's callback function with an error response. The
   * callback function is not cleared
   *
   * @param error_msg error message to send back to the client
   */
  void runCallbackError(std::string_view error_msg);

  void addInputTensor(void *data, std::vector<uint64_t> shape,
                      DataType dataType, std::string name = "");

  void addInputTensor(InferenceRequestInput input);
  void addOutputTensor(InferenceRequestOutput output);

  /// Get a vector of all the input request objects
  const std::vector<InferenceRequestInput> &getInputs() const;
  /// Get the number of input request objects
  size_t getInputSize();

  /// Get a vector of the requested output information
  const std::vector<InferenceRequestOutput> &getOutputs() const;

  /**
   * @brief Get the ID associated with this request
   *
   * @return std::string
   */
  const std::string &getID() const { return id_; }
  void setID(const std::string &id) { id_ = id; }

  /// Get a pointer to the request's parameters
  RequestParameters *getParameters() const { return this->parameters_.get(); }
  void setParameters(RequestParametersPtr parameters) {
    parameters_ = parameters;
  }

 private:
  std::string id_;
  RequestParametersPtr parameters_;
  std::vector<InferenceRequestInput> inputs_;
  std::vector<InferenceRequestOutput> outputs_;
  Callback callback_;

  // TODO(varunsh): do we need this still?
  friend class FakeInferenceRequest;
  template <typename U>
  friend class InferenceRequestBuilder;
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

  const std::string &getName() const;
  const DataType &getDataType() const;
  const std::vector<uint64_t> &getShape() const;

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
   * @brief Construct a new Model Metadata object
   *
   * @param name Name of the model
   * @param platform the platform this model runs on
   */
  ModelMetadata(const std::string &name, const std::string &platform);

  /**
   * @brief Add an input tensor to this model
   *
   * @param name name of the tensor
   * @param datatype datatype of the tensor
   * @param shape shape of the tensor
   */
  void addInputTensor(const std::string &name, DataType datatype,
                      std::initializer_list<uint64_t> shape);
  /**
   * @brief Add an input tensor to this model
   *
   * @param name name of the tensor
   * @param datatype datatype of the tensor
   * @param shape shape of the tensor
   */
  void addInputTensor(const std::string &name, DataType datatype,
                      std::vector<int> shape);

  const std::vector<ModelMetadataTensor> &getInputs() const;

  /**
   * @brief Add an output tensor to this model
   *
   * @param name name of the tensor
   * @param datatype datatype of the tensor
   * @param shape shape of the tensor
   */
  void addOutputTensor(const std::string &name, DataType datatype,
                       std::initializer_list<uint64_t> shape);
  /**
   * @brief Add an output tensor to this model
   *
   * @param name name of the tensor
   * @param datatype datatype of the tensor
   * @param shape shape of the tensor
   */
  void addOutputTensor(const std::string &name, DataType datatype,
                       std::vector<int> shape);

  const std::vector<ModelMetadataTensor> &getOutputs() const;

  /// set the model's name
  void setName(const std::string &name);
  /// get the model's name
  const std::string &getName() const;

  const std::string &getPlatform() const;

  /// mark this model as ready/not ready
  void setReady(bool ready);
  /// check if this model is ready
  [[nodiscard]] bool isReady() const;

 private:
  std::string name_;
  std::vector<std::string> versions_;
  std::string platform_;
  std::vector<ModelMetadataTensor> inputs_;
  std::vector<ModelMetadataTensor> outputs_;
  bool ready_;
};

}  // namespace proteus

namespace std {
template <>
/**
 * @brief Overload the "less than" operator so we can compare two
 * RequestParameter objects. We need this functionality to store objects of
 * this class in a map. Note, since hashing is not implemented, these objects
 * cannot be stored in an unordered_map.
 *
 */
struct less<proteus::RequestParameters> {
  /**
   * @brief Implementation of the comparison of two RequestParameter objects.
   * We compare the size and then check each key is present and finally, compare
   * the key values. The types supported in RequestParameters all support
   * direct comparison with the "less than" operator already.
   *
   * @param lhs the RequestParameter object on the left-hand-side
   * @param rhs the RequestParameter object on the right-hand-side
   * @return bool
   */
  bool operator()(const proteus::RequestParameters &lhs,
                  const proteus::RequestParameters &rhs) const {
    auto lhs_size = lhs.size();
    auto rhs_size = rhs.size();
    auto lhs_map = lhs.data();
    auto rhs_map = rhs.data();
    if (lhs_size == rhs_size) {
      for (auto &[key, lhs_value] : lhs_map) {
        if (rhs_map.find(key) == rhs_map.end()) {
          return true;
        }
        auto rhs_value = rhs_map.at(key);
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

#endif  // GUARD_PROTEUS_CORE_PREDICT_API
