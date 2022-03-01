// Copyright 2022 Xilinx Inc.
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
 * @brief Defines the internal objects used for gRPC
 */

#ifndef GUARD_PROTEUS_CLIENTS_GRPC_INTERNAL
#define GUARD_PROTEUS_CLIENTS_GRPC_INTERNAL

#include "proteus/core/predict_api_internal.hpp"

namespace proteus {

class CallDataModelInfer;

template <>
class InferenceRequestBuilder<CallDataModelInfer *> {
 public:
  static InferenceRequestPtr build(
    const CallDataModelInfer *req, size_t &buffer_index,
    const std::vector<BufferRawPtrs> &input_buffers,
    std::vector<size_t> &input_offsets,
    const std::vector<BufferRawPtrs> &output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset);
};

using RequestBuilder = InferenceRequestBuilder<CallDataModelInfer *>;

}  // namespace proteus

#endif  // GUARD_PROTEUS_CLIENTS_GRPC_INTERNAL
