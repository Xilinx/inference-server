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
 * @brief Implements the associated containers for the Parameter object
 */

#include "amdinfer/core/parameters.hpp"

#include <cassert>      // for assert
#include <cstdint>      // for int32_t
#include <cstring>      // for size_t, memcpy
#include <tuple>        // for tuple
#include <type_traits>  // for add_const<>::type, decay_t
#include <vector>       // for vector

#include "amdinfer/util/memory.hpp"  // for copy

namespace amdinfer {

void ParameterMap::put(const std::string &key, bool value) {
  this->parameters_.try_emplace(key, value);
}

void ParameterMap::put(const std::string &key, double value) {
  this->parameters_.try_emplace(key, value);
}

void ParameterMap::put(const std::string &key, int32_t value) {
  this->parameters_.try_emplace(key, value);
}

void ParameterMap::put(const std::string &key, const std::string &value) {
  this->parameters_.try_emplace(key, value);
}

void ParameterMap::put(const std::string &key, const char *value) {
  this->parameters_.try_emplace(key, std::string{value});
}

void ParameterMap::erase(const std::string &key) {
  if (this->parameters_.find(key) != this->parameters_.end()) {
    this->parameters_.erase(key);
  }
}

bool ParameterMap::has(const std::string &key) {
  return this->parameters_.find(key) != this->parameters_.end();
}

size_t ParameterMap::size() const { return parameters_.size(); }

bool ParameterMap::empty() const { return parameters_.empty(); }

ParameterMap::Iterator ParameterMap::begin() { return parameters_.begin(); }
ParameterMap::ConstIterator ParameterMap::cbegin() const {
  return parameters_.cbegin();
}

ParameterMap::Iterator ParameterMap::end() { return parameters_.end(); }
ParameterMap::ConstIterator ParameterMap::cend() const {
  return parameters_.cend();
}

std::map<std::string, Parameter, std::less<>> ParameterMap::data() const {
  return parameters_;
}

size_t ParameterMap::serializeSize() const {
  // 1 for num of parameters plus 3 for each parameter for type index, key size
  // and value size
  auto size = ((this->size() * 3) + 1) * sizeof(size_t);
  for (const auto &[key, value] : parameters_) {
    size += key.size();
    std::visit(
      [&size](const auto &param) {
        using T = std::decay_t<decltype(param)>;
        if constexpr (std::is_same_v<T, std::string>) {
          size += param.size();
        } else {
          size += sizeof(param);
        }
      },
      value);
  }
  return size;
}

void ParameterMap::serialize(std::byte *data_out) const {
  auto size = this->size();
  std::string foo;
  data_out = copy(size, data_out, sizeof(size_t));
  for (const auto &[key, value] : parameters_) {
    data_out = copy(value.index(), data_out, sizeof(size_t));
    data_out = copy(key.size(), data_out, sizeof(size_t));
    std::visit(
      [&](const auto &param) {
        using T = std::decay_t<decltype(param)>;
        if constexpr (std::is_same_v<T, std::string>) {
          data_out = copy(param.size(), data_out, sizeof(size_t));
        } else {
          data_out = copy(sizeof(param), data_out, sizeof(size_t));
        }
      },
      value);
  }
  for (const auto &[key, value] : parameters_) {
    data_out = copy(key.c_str(), data_out, key.size());
    std::visit(
      [&](const auto &param) {
        using T = std::decay_t<decltype(param)>;
        if constexpr (std::is_same_v<T, std::string>) {
          data_out = copy(param.c_str(), data_out, param.size());
        } else {
          data_out = copy(param, data_out, sizeof(param));
        }
      },
      value);
  }
}

/**
 * @brief This is used to turn a runtime index into a variant with the right
 * type
 *
 * @tparam Ts: the types of the variant. Order matters!
 * @param i index of the variant type to create
 * @return std::variant<Ts...>
 *
 * https://www.reddit.com/r/cpp/comments/f8cbzs/comment/fimjm2f/?context=3
 */
template <typename... Ts>
[[nodiscard]] std::variant<Ts...> expandType(std::size_t i) {
  assert(i < sizeof...(Ts));
  static constexpr auto kTable =
    std::array{+[]() { return std::variant<Ts...>{Ts{}}; }...};
  return kTable.at(i)();
}

void ParameterMap::deserialize(const std::byte *data_in) {
  parameters_.clear();

  auto size = std::to_integer<size_t>(*data_in);
  data_in += sizeof(size_t);
  std::vector<std::tuple<size_t, size_t, size_t>> params;
  params.reserve(size);
  for (auto i = 0U; i < size; i++) {
    auto index = std::to_integer<size_t>(*data_in);
    data_in += sizeof(size_t);
    auto key_size = std::to_integer<size_t>(*data_in);
    data_in += sizeof(size_t);
    auto data_size = std::to_integer<size_t>(*data_in);
    data_in += sizeof(size_t);
    params.emplace_back(index, key_size, data_size);
  }
  for (const auto &[index, key_size, data_size] : params) {
    std::string key;
    key.resize(key_size);
    std::memcpy(key.data(), data_in, key_size);
    data_in += key_size;
    Parameter param = expandType<bool, int32_t, double, std::string>(index);
    const auto &n = data_size;
    std::visit(
      [&](auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::string>) {
          value.resize(n);
          std::memcpy(value.data(), data_in, n);
          data_in += n;
        } else {
          std::memcpy(&value, data_in, n);
          data_in += n;
        }
      },
      param);
    parameters_.try_emplace(key, param);
  }
}

}  // namespace amdinfer
