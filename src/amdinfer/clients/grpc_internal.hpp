// Copyright 2022 Xilinx, Inc.
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
 * @brief Defines the internal objects used for gRPC
 */

#ifndef GUARD_AMDINFER_CLIENTS_GRPC_INTERNAL
#define GUARD_AMDINFER_CLIENTS_GRPC_INTERNAL

#include <map>
#include <string>

#include "amdinfer/core/interface.hpp"  // for Interface
#include "amdinfer/core/predict_api_internal.hpp"
#include "amdinfer/observation/observer.hpp"  // for Observer
#include "amdinfer/util/traits.hpp"           // for is_any

namespace google::protobuf {
template <typename T, typename U>
class Map;
}  // namespace google::protobuf

namespace inference {
class InferParameter;
class ModelInferResponse;
class ModelMetadataResponse;
class ModelInferRequest;
}  // namespace inference

namespace amdinfer {

void mapParametersToProto(
  const std::map<std::string, amdinfer::Parameter, std::less<>>& parameters,
  google::protobuf::Map<std::string, inference::InferParameter>*
    grpc_parameters);
RequestParametersPtr mapProtoToParameters(
  const google::protobuf::Map<std::string, inference::InferParameter>& params);
void mapProtoToParameters(
  const google::protobuf::Map<std::string, inference::InferParameter>& params,
  RequestParameters& parameters);
void mapRequestToProto(const InferenceRequest& request,
                       inference::ModelInferRequest& grpc_request,
                       const Observer& observer);
void mapResponseToProto(InferenceResponse response,
                        inference::ModelInferResponse& reply);
void mapProtoToResponse(const inference::ModelInferResponse& reply,
                        InferenceResponse& response, const Observer& observer);

void mapModelMetadataToProto(const ModelMetadata& metadata,
                             inference::ModelMetadataResponse& resp);

template <typename T, typename Tensor>
constexpr auto* getTensorContents(Tensor* tensor) {
  if constexpr (std::is_same_v<T, bool>) {
    if constexpr (std::is_const_v<Tensor>) {
      return tensor->contents().bool_contents().data();
    } else {
      return tensor->mutable_contents()->mutable_bool_contents();
    }
  } else if constexpr (util::is_any_v<T, uint8_t, uint16_t, uint32_t>) {
    if constexpr (std::is_const_v<Tensor>) {
      return tensor->contents().uint_contents().data();
    } else {
      return tensor->mutable_contents()->mutable_uint_contents();
    }
  } else if constexpr (std::is_same_v<T, uint64_t>) {
    if constexpr (std::is_const_v<Tensor>) {
      return tensor->contents().uint64_contents().data();
    } else {
      return tensor->mutable_contents()->mutable_uint64_contents();
    }
  } else if constexpr (util::is_any_v<T, int8_t, int16_t, int32_t>) {
    if constexpr (std::is_const_v<Tensor>) {
      return tensor->contents().int_contents().data();
    } else {
      return tensor->mutable_contents()->mutable_int_contents();
    }
  } else if constexpr (std::is_same_v<T, int64_t>) {
    if constexpr (std::is_const_v<Tensor>) {
      return tensor->contents().int64_contents().data();
    } else {
      return tensor->mutable_contents()->mutable_int64_contents();
    }
  } else if constexpr (util::is_any_v<T, fp16, float>) {
    if constexpr (std::is_const_v<Tensor>) {
      return tensor->contents().fp32_contents().data();
    } else {
      return tensor->mutable_contents()->mutable_fp32_contents();
    }
  } else if constexpr (std::is_same_v<T, double>) {
    if constexpr (std::is_const_v<Tensor>) {
      return tensor->contents().fp64_contents().data();
    } else {
      return tensor->mutable_contents()->mutable_fp64_contents();
    }
  } else if constexpr (std::is_same_v<T, char>) {
    if constexpr (std::is_const_v<Tensor>) {
      return tensor->contents().bytes_contents().data();
    } else {
      return tensor->mutable_contents()->mutable_bytes_contents();
    }
  } else {
    static_assert(!sizeof(T), "Invalid type to AddDataToTensor");
  }
}

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CLIENTS_GRPC_INTERNAL
