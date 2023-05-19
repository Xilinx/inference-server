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
 * @brief Defines the Parameter object and associated containers
 */

#include <cstddef>     // for byte, size_t
#include <cstdint>     // for int32_t
#include <functional>  // for less
#include <map>         // for map
#include <memory>      // for shared_ptr
#include <sstream>     // for operator<<, basic_ostream, strin...
#include <string>      // for string, operator<<, char_traits
#include <variant>     // for visit, variant

#include "amdinfer/core/mixins.hpp"  // for Serializable

#ifndef GUARD_AMDINFER_CORE_PARAMETERS
#define GUARD_AMDINFER_CORE_PARAMETERS

namespace amdinfer {

/// parameters may be one of these types
using Parameter = std::variant<bool, int32_t, double, std::string>;

/**
 * @brief Holds any parameters from JSON (defined by KServe spec as one of
 * bool, number or string). We further restrict numbers to be doubles or int32.
 *
 */
class ParameterMap : public Serializable {
  using Container = std::map<std::string, Parameter, std::less<>>;
  using Iterator = Container::iterator;
  using ConstIterator = Container::const_iterator;

 public:
  ParameterMap() = default;
  /**
   * @brief Construct a new ParameterMap object with initial values. The sizes
   * of the keys and values vectors must match.
   *
   * @details Until C++20, passing const char* to this constructor will convert
   * it to a bool instead of a string. Explicitly convert any string literals to
   * a string before passing them to this constructor.
   *
   * @param keys
   * @param values
   */
  ParameterMap(const std::vector<std::string> &keys,
               const std::vector<Parameter> &values);

  /**
   * @brief Put in a key-value pair
   *
   * @param key key used to store and retrieve the value
   * @param value value to store
   */
  void put(const std::string &key, const Parameter &value);
  /**
   * @brief Put in a key-value pair
   *
   * @details This overload is needed because C++ converts const char* to bool
   * instead of string when both types are present in the variant. This behavior
   * has been fixed in C++20.
   *
   * @param key
   * @param value
   */
  void put(const std::string &key, const char *value);
  /**
   * @brief Get the named parameter
   *
   * @tparam T type of parameter. Must be (bool|double|int32_t|std::string)
   * @param key parameter to get
   * @return T
   */
  template <typename T>
  T get(const std::string &key) const {
    auto &value = this->parameters_.at(key);
    return std::get<T>(value);
  }

  /**
   * @brief Checks if a particular parameter exists
   *
   * @param key name of the parameter to check
   * @return bool
   */
  bool has(const std::string &key) const;

  /**
   * @brief Rename the key associated with a parameter. If the new key already
   * exists, its value is not overwritten and the old key is just erased.
   *
   * @param key
   * @param new_key
   */
  void rename(const std::string &key, const std::string &new_key);

  /**
   * @brief Removes a parameter, if it exists. No error is raised if it doesn't
   * exist
   *
   * @param key name of the parameter to remove
   */
  void erase(const std::string &key);
  /// Gets the number of parameters
  [[nodiscard]] size_t size() const;
  /// Checks if the parameters are empty
  [[nodiscard]] bool empty() const;
  /// Gets the underlying data structure holding the parameters
  [[nodiscard]] std::map<std::string, Parameter, std::less<>> data() const;

  /// Returns a read/write iterator to the first parameter in the object
  Iterator begin();
  /// Returns a read iterator to the first parameter in the object
  [[nodiscard]] ConstIterator begin() const;
  /// Returns a read iterator to the first parameter in the object
  [[nodiscard]] ConstIterator cbegin() const;

  /// Returns a read/write iterator to one past the last parameter in the object
  Iterator end();
  /// Returns a read iterator to one past the last parameter in the object
  [[nodiscard]] ConstIterator end() const;
  /// Returns a read iterator to one past the last parameter in the object
  [[nodiscard]] ConstIterator cend() const;

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

  /// Provides an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os, const ParameterMap &self) {
    std::stringstream ss;
    ss << "ParameterMap(" << &self << "):\n";
    for (const auto &[key, value] : self.parameters_) {
      ss << "  " << key << ": ";
      std::visit([&](const auto &c) { ss << c; }, value);
      ss << "\n";
    }
    auto tmp = ss.str();
    tmp.pop_back();  // delete trailing newline
    os << tmp;
    return os;
  }

 private:
  Container parameters_;
};

using ParameterMapPtr = std::shared_ptr<ParameterMap>;

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

#endif  // GUARD_AMDINFER_CORE_PARAMETERS
