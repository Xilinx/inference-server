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
 * @brief Defines base64 encoding/decoding
 */

#ifndef GUARD_PROTEUS_HELPERS_BASE64
#define GUARD_PROTEUS_HELPERS_BASE64

#include <cstddef>  // for size_t
#include <string>   // for string

namespace proteus {

/**
 * @brief Decodes a base64-encoded string and returns it
 *
 * @param in the encoded string
 * @return std::string decoded string
 */
std::string base64_decode(std::string in);

/**
 * @brief Decodes a base64-encoded string and returns it
 *
 * @param in char* to the encoded string
 * @param in_len length of the encoded string
 * @return std::string decoded string
 */
std::string base64_decode(const char* in, size_t in_len);

/**
 * @brief Encodes a string with base64 and returns it
 *
 * @param in string to encode
 * @return std::string encoded string
 */
std::string base64_encode(std::string in);

/**
 * @brief Encodes a string with base64 and returns it
 *
 * @param in char* to the string
 * @param in_len length of the string
 * @return std::string encoded string
 */
std::string base64_encode(const char* in, size_t in_len);
}  // namespace proteus

#endif  // GUARD_PROTEUS_HELPERS_BASE64
