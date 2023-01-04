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
 * @brief Defines the internal objects used to hold inference requests and
 * responses
 */

#ifndef GUARD_AMDINFER_CORE_PREDICT_API_INTERNAL
#define GUARD_AMDINFER_CORE_PREDICT_API_INTERNAL

#include "amdinfer/core/predict_api.hpp"  // IWYU pragma: export

namespace amdinfer {

template <typename T>
class InferenceRequestBuilder {
 public:
  /**
   * @brief Construct a new InferenceRequest object
   *
   * @param req some valid type holding data
   * @param buffer_index current buffer index to start with for the buffers
   * @param input_buffers a vector of input buffers to store the inputs data
   * @param input_offsets a vector of offsets for the input buffers to store
   * data
   * @param output_buffers a vector of output buffers
   * @param output_offsets a vector of offsets for the output buffers
   * @param batch_size batch size to use when creating the request
   * @param batch_offset current batch offset to start with
   */
  static InferenceRequestPtr build(
    T req, size_t &buffer_index,
    const std::vector<BufferRawPtrs> &input_buffers,
    std::vector<size_t> &input_offsets,
    const std::vector<BufferRawPtrs> &output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset);
};

template <typename T>
class InferenceRequestInputBuilder {
 public:
  /**
   * @brief Construct a new InferenceRequestInput object
   *
   * @param req some valid type holding data
   * @param input_buffer buffer to hold the incoming data
   * @param offset offset for the buffer to store data at
   */
  static InferenceRequestInput build(const T &req, Buffer *input_buffer,
                                     size_t offset);
};

template <typename T>
class InferenceRequestOutputBuilder {
 public:
  /**
   * @brief Construct a new InferenceRequestOutput object
   *
   * @param req some valid type holding data
   * @param input_buffer buffer to hold the incoming data
   * @param offset offset for the buffer to store data at
   */
  static InferenceRequestOutput build(const T &req);
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_PREDICT_API_INTERNAL
