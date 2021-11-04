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
#include <string>            // for string, operator<<, char...
#include <utility>           // for move
#include <variant>           // for operator!=, operator<
#include <vector>            // for vector

#include "proteus/core/data_types.hpp"       // for DataType, mapTypeToStr
#include "proteus/helpers/declarations.hpp"  // for InferenceResponseOutput

namespace Json {
class Value;
}  // namespace Json

namespace proteus {
class Buffer;
}  // namespace proteus

namespace proteus {

// parameters in Proteus may be one of these types
using Parameter = std::variant<bool, double, int32_t, std::string>;

/**
 * @brief Holds any parameters from JSON (defined by KFserving spec as one of
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
    auto value = this->parameters_.at(key);
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

  /// Provide an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os,
                                  RequestParameters const &my_class) {
    for (const auto &[key, value] : my_class.parameters_) {
      os << key << ": ";
      std::visit([&os](auto &&c) { os << c; }, value);
      os << "\n";
    }
    return os;
  }

 private:
  std::map<std::string, Parameter> parameters_;
};

using RequestParametersPtr = std::shared_ptr<RequestParameters>;

template <>
Parameter *RequestParameters::get(const std::string &key);

/**
 * @brief Convert JSON-styled parameters to Proteus's implementation
 *
 * @param parameters
 * @return RequestParametersPtr
 */
RequestParametersPtr addParameters(Json::Value parameters);

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
   * @param req the JSON request from the user
   * @param input_buffer buffer to hold the incoming data
   * @param offset offset for the buffer to store data at
   */
  InferenceRequestInput(std::shared_ptr<Json::Value> const &req,
                        Buffer *input_buffer, size_t offset);
  /**
   * @brief Construct a new InferenceRequestInput object
   *
   * @param req an existing InferenceRequestInput to copy
   * @param input_buffer buffer to hold the incoming data
   * @param offset offset for the buffer to store data at
   */
  InferenceRequestInput(InferenceRequestInput &req, Buffer *input_buffer,
                        size_t offset);

  /**
   * @brief Construct a new InferenceRequestInput object
   *
   * @param data pointer to data
   * @param shape shape of the data
   * @param dataType type of the data
   * @param name name to assign
   */
  InferenceRequestInput(void *data, std::vector<uint64_t> shape,
                        types::DataType dataType, std::string name = "");

  /// Set the request's data
  void setData(void *buffer) { this->data_ = buffer; }
  /// Set the request's shared data
  void setData(std::shared_ptr<std::byte> buffer) {
    this->shared_data_ = std::move(buffer);
  }

  /// Get a pointer to the request's data
  [[nodiscard]] void *getData() const;

  /// Get the input tensor's name
  std::string getName() { return this->name_; }
  /// Set the input tensor's name
  void setName(std::string name);

  /// Get the input tensor's shape
  std::vector<uint64_t> getShape() { return this->shape_; }
  /// Set the tensor's shape
  void setShape(std::initializer_list<uint64_t> shape) { this->shape_ = shape; }
  /// Set the tensor's shape
  void setShape(const std::vector<uint64_t> &shape) {
    this->shape_ = std::move(shape);
  }
  /// Set the tensor's shape
  void setShape(std::vector<int32_t> shape) {
    this->shape_.reserve(shape.size());
    for (const auto &index : shape) {
      this->shape_.push_back(index);
    }
  }

  /// Get the input tensor's datatype
  types::DataType getDatatype() { return this->dataType_; }
  /// Set the tensor's data type
  void setDatatype(types::DataType type);

  /// Get the input tensor's parameters
  RequestParameters *getParameters() { return this->parameters_.get(); }

  /// Set the tensor's size
  size_t getSize();

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
    os << "  Datatype: " << types::mapTypeToStr(my_class.dataType_) << "\n";
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
  types::DataType dataType_;
  RequestParametersPtr parameters_;
  void *data_;
  std::shared_ptr<std::byte> shared_data_;
};

/**
 * @brief Holds an inference request's output data
 *
 */
class InferenceRequestOutput {
 public:
  /// Construct a new Request Output object
  InferenceRequestOutput();

  /// Construct a new Request Output object
  explicit InferenceRequestOutput(std::shared_ptr<Json::Value> const &req);

  /// Set the request's data
  void setData(void *buffer) { this->data_ = buffer; }

  /// Take the request's data
  void *getData() { return this->data_; }

  /// Get the output tensor's name
  std::string getName() { return this->name_; }
  void setName(const std::string &name);

 private:
  std::string name_;
  RequestParametersPtr parameters_;
  void *data_;
};

/**
 * @brief Creates an inference response object based on KFserving's V2 spec that
 * is used to respond back to clients.
 */
class InferenceResponse {
 public:
  /// Construct a new InferenceResponse object
  explicit InferenceResponse();

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

  /// Get a pointer to the parameters associated with this response
  RequestParameters *getParameters() { return this->parameters_.get(); }

  /// Provide an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os,
                                  InferenceResponse const &my_class) {
    os << "Inference Response:\n";
    os << "  Model: " << my_class.model_ << "\n";
    os << "  ID: " << my_class.id_ << "\n";
    os << "  Parameters:\n";
    os << *(my_class.parameters_.get()) << "\n";
    os << "  Outputs:\n";
    for (const auto &output : my_class.outputs_) {
      os << output << "\n";
    }
    return os;
  }

 private:
  std::string model_;
  std::string id_;
  std::shared_ptr<RequestParameters> parameters_;
  std::vector<InferenceResponseOutput> outputs_;
};

using Callback = std::function<void(const InferenceResponse &)>;

/**
 * @brief Creates an inference request object based on KFserving's V2 spec that
 * is used to communicate between workers in Proteus
 */
class InferenceRequest {
 public:
  // Construct a new InferenceRequest object
  InferenceRequest() = default;
  /**
   * @brief Partially construct a new InferenceRequest object
   *
   * @param req the JSON request received from the client
   */
  explicit InferenceRequest(std::shared_ptr<Json::Value> const &req);

  /**
   * @brief Construct a new InferenceRequest object
   *
   * @param req one input request object
   * @param buffer_index current buffer index to start with for the buffers
   * @param input_buffers a vector of input buffers to store the inputs data
   * @param input_offsets a vector of offsets for the input buffers to store
   * data
   * @param output_buffers a vector of output buffers
   * @param output_offsets a vector of offsets for the output buffers
   * @param batch_size batch size to use when creating the request
   * @param batch_offset current batch offset to start with
   */
  InferenceRequest(InferenceRequestInput &req, size_t &buffer_index,
                   std::vector<BufferRawPtrs> input_buffers,
                   std::vector<size_t> &input_offsets,
                   std::vector<BufferRawPtrs> output_buffers,
                   std::vector<size_t> &output_offsets,
                   const size_t &batch_size, size_t &batch_offset);

  /**
   * @brief Construct a new InferenceRequest object
   *
   * @param req JSON request from the client
   * @param buffer_index current buffer index to start with for the buffers
   * @param input_buffers a vector of input buffers to store the inputs data
   * @param input_offsets a vector of offsets for the input buffers to store
   * data
   * @param output_buffers a vector of output buffers
   * @param output_offsets a vector of offsets for the output buffers
   * @param batch_size batch size to use when creating the request
   * @param batch_offset current batch offset to start with
   */
  InferenceRequest(std::shared_ptr<Json::Value> const &req,
                   size_t &buffer_index,
                   std::vector<BufferRawPtrs> input_buffers,
                   std::vector<size_t> &input_offsets,
                   std::vector<BufferRawPtrs> output_buffers,
                   std::vector<size_t> &output_offsets,
                   const size_t &batch_size, size_t &batch_offset);

  /**
   * @brief Set the request's callback function used by the last worker to
   * respond back to the client
   *
   * @param callback a function pointer that accepts a InferenceResponse object
   */
  void setCallback(Callback &&callback);
  /**
   * @brief Get the request's callback function
   *
   * @return Callback
   */
  Callback getCallback();

  /// Get a vector of all the input request objects
  std::vector<InferenceRequestInput> getInputs();
  /// Get the number of input request objects
  size_t getInputSize();

  /// Get a vector of the requested output information
  std::vector<InferenceRequestOutput> getOutputs();

  /**
   * @brief Get the ID associated with this request
   *
   * @return std::string
   */
  std::string getID() { return id_; }

  /// Get a pointer to the request's parameters
  RequestParameters *getParameters() { return this->parameters_.get(); }

 private:
  std::string id_;
  RequestParametersPtr parameters_;
  std::vector<InferenceRequestInput> inputs_;
  std::vector<InferenceRequestOutput> outputs_;
  Callback callback_;

  // TODO(varunsh): do we need this still?
  friend class FakeInferenceRequest;
};
using InferenceResponsePromisePtr =
  std::shared_ptr<std::promise<InferenceResponse>>;

}  // namespace proteus

namespace std {
template <>
struct less<proteus::RequestParameters> {
  /**
   * @brief Overload the "less than" operator so we can compare two
   * RequestParameter objects. We need this functionality to store objects of
   * this class in a map. Note, since hashing is not implemented, these objects
   * cannot be stored in an unordered_map.
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
