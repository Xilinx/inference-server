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
 * @brief Defines the internal objects used to hold inference requests and
 * responses
 */

#ifndef GUARD_PROTEUS_CORE_PREDICT_API_INTERNAL
#define GUARD_PROTEUS_CORE_PREDICT_API_INTERNAL

#include "proteus/core/predict_api.hpp"  // IWYU pragma: export

namespace Json {
class Value;
}

namespace proteus {

class CallDataModelInfer;

/**
 * @brief Convert JSON-styled parameters to Proteus's implementation
 *
 * @param parameters
 * @return RequestParametersPtr
 */
RequestParametersPtr addParameters(Json::Value parameters);

class InferenceRequestOutputBuilder {
 public:
  static InferenceRequestOutput fromJson(
    std::shared_ptr<Json::Value> const &req);
};

class InferenceRequestBuilder {
 public:
  /**
   * @brief Construct a new InferenceRequest object
   *
   * @param req one inference request object
   * @param buffer_index current buffer index to start with for the buffers
   * @param input_buffers a vector of input buffers to store the inputs data
   * @param input_offsets a vector of offsets for the input buffers to store
   * data
   * @param output_buffers a vector of output buffers
   * @param output_offsets a vector of offsets for the output buffers
   * @param batch_size batch size to use when creating the request
   * @param batch_offset current batch offset to start with
   */
  static InferenceRequestPtr fromInput(
    InferenceRequest &req, size_t &buffer_index,
    const std::vector<BufferRawPtrs> &input_buffers,
    std::vector<size_t> &input_offsets,
    const std::vector<BufferRawPtrs> &output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset);

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
  static InferenceRequestPtr fromJson(
    std::shared_ptr<Json::Value> const &req, size_t &buffer_index,
    const std::vector<BufferRawPtrs> &input_buffers,
    std::vector<size_t> &input_offsets,
    const std::vector<BufferRawPtrs> &output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset);

  /**
   * @brief Construct a new InferenceRequest object
   *
   * @param req one inference request object
   * @param buffer_index current buffer index to start with for the buffers
   * @param input_buffers a vector of input buffers to store the inputs data
   * @param input_offsets a vector of offsets for the input buffers to store
   * data
   * @param output_buffers a vector of output buffers
   * @param output_offsets a vector of offsets for the output buffers
   * @param batch_size batch size to use when creating the request
   * @param batch_offset current batch offset to start with
   */
  static InferenceRequestPtr fromGrpc(
    CallDataModelInfer *calldata, size_t &buffer_index,
    const std::vector<BufferRawPtrs> &input_buffers,
    std::vector<size_t> &input_offsets,
    const std::vector<BufferRawPtrs> &output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset);
};

/// convert the metadata to a JSON representation compatible with the server
Json::Value ModelMetadataToJson(const ModelMetadata &metadata);

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_PREDICT_API_INTERNAL
