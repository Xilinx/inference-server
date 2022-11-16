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
 * @brief Implements base64 encoding/decoding
 */

#include "amdinfer/util/base64.hpp"

#include <stdexcept>

// this define is needed for compilation but its value isn't used here
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUFFERSIZE 4096
#include <b64/decode.h>
#include <b64/encode.h>

namespace amdinfer::util {

constexpr size_t minDecodeLength(size_t length) { return (length * 3 + 3) / 4; }

std::string base64_decode(std::string in) {
  return base64_decode(in.data(), in.length());
}

std::string base64_decode(const char* in, size_t in_length) {
  std::string s;
  s.resize(minDecodeLength(in_length));

  base64::base64_decodestate state_;
  base64::base64_init_decodestate(&state_);
  int out_length =
    base64::base64_decode_block(in, in_length, s.data(), &state_);

  if (out_length < 0) {
    throw std::length_error("Base64 decoded string failed");
  }

  s.resize(out_length);
  return s;
}

/**
 * @brief Compute the minimum space needed for a base64-encoded string. Base64
 * produces 4 output bytes for every 3 input bytes.
 *
 * @param length length of the string to encode
 * @return constexpr size_t
 */
constexpr size_t minEncodeLength(size_t length) {
  return (((4 * length / 3) + 3) & ~3);
}

std::string base64_encode(std::string in) {
  return base64_encode(in.data(), in.length());
}

std::string base64_encode(const char* in, size_t in_length) {
  std::string s;
  s.resize(minEncodeLength(in_length));

  base64::base64_encodestate state;
  base64::base64_init_encodestate(&state);
  int codelength = base64::base64_encode_block(in, in_length, s.data(), &state);
  codelength += base64::base64_encode_blockend(s.data() + codelength, &state);

  if (codelength < 0) {
    throw std::length_error("Base64 encoding string failed");
  }

  s.resize(codelength);
  return s;
}

}  // namespace amdinfer::util
