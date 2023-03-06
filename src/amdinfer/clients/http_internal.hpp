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
 * @brief Defines the internal objects used for HTTP/REST
 */

#ifndef GUARD_AMDINFER_CLIENTS_HTTP_INTERNAL
#define GUARD_AMDINFER_CLIENTS_HTTP_INTERNAL

#include <drogon/HttpRequest.h>   // for HttpRequestPtr
#include <drogon/HttpResponse.h>  // for HttpResponsePtr
#include <json/value.h>           // for Value

#include <cstddef>     // for size_t
#include <exception>   // for invalid_argument
#include <functional>  // for function
#include <memory>      // for shared_ptr
#include <string>      // for string
#include <vector>      // for vector

#include "amdinfer/build_options.hpp"              // for AMDINFER_ENABLE_TRA...
#include "amdinfer/core/parameters.hpp"            // for ParameterMap (ptr ...
#include "amdinfer/core/predict_api_internal.hpp"  // for InferenceRequestBui...
#include "amdinfer/core/protocol_wrapper.hpp"      // for ProtocolWrapper
#include "amdinfer/declarations.hpp"               // for BufferRawPtrs, Infe...
#include "amdinfer/util/traits.hpp"                // for is_any_v

namespace amdinfer {

/**
 * @brief Convert JSON-styled parameters to our objects
 *
 * @param parameters
 * @return ParameterMapPtr
 */
ParameterMapPtr mapJsonToParameters(Json::Value json);
Json::Value mapParametersToJson(ParameterMap *parameters);

InferenceResponse mapJsonToResponse(Json::Value *json);
Json::Value mapRequestToJson(const InferenceRequest &request);

template <>
class InferenceRequestBuilder<std::shared_ptr<Json::Value>> {
 public:
  static InferenceRequestPtr build(const std::shared_ptr<Json::Value> &req,
                                   const BufferRawPtrs &input_buffers,
                                   std::vector<size_t> &input_offsets,
                                   const BufferRawPtrs &output_buffers,
                                   std::vector<size_t> &output_offsets);
};

using RequestBuilder = InferenceRequestBuilder<std::shared_ptr<Json::Value>>;

#ifdef AMDINFER_ENABLE_TRACING
void propagate(drogon::HttpResponse *resp, const StringMap &context);
#endif

struct SetInputData {
  template <typename T>
  void operator()(Json::Value *json, void *src_data, size_t src_size) const {
    auto *data = static_cast<T *>(src_data);
    if constexpr (std::is_same_v<T, char>) {
      std::string str{data, src_size};
      json->append(str);
    } else {
      // NOLINTNEXTLINE(readability-identifier-naming)
      constexpr auto getData = [](const T *data_ptr, size_t index) {
        if constexpr (std::is_same_v<T, uint64_t>) {
          return static_cast<Json::UInt64>(data_ptr[index]);
        } else if constexpr (std::is_same_v<T, int64_t>) {
          return static_cast<Json::Int64>(data_ptr[index]);
        } else if constexpr (util::is_any_v<T, bool, uint8_t, uint16_t,
                                            uint32_t, int8_t, int16_t, int32_t,
                                            float, double>) {
          return data_ptr[index];
        } else if constexpr (util::is_any_v<T, fp16>) {
          return half_float::half_cast<float>(data_ptr[index]);
        } else {
          static_assert(!sizeof(T), "Invalid type to SetInputData");
        }
      };

      for (auto i = 0U; i < src_size; ++i) {
        json->append(getData(data, i));
      }
    }
  }
};

template <typename T>
constexpr auto jsonValueToType(const Json::Value &datum) {
  if constexpr (std::is_same_v<T, bool>) {
    return datum.asBool();
  } else if constexpr (util::is_any_v<T, uint8_t, uint16_t, uint32_t>) {
    return datum.asUInt();
  } else if constexpr (std::is_same_v<T, uint64_t>) {
    return datum.asUInt64();
  } else if constexpr (util::is_any_v<T, int8_t, int16_t, int32_t>) {
    return datum.asInt();
  } else if constexpr (std::is_same_v<T, int64_t>) {
    return datum.asInt64();
  } else if constexpr (util::is_any_v<T, fp16, float>) {
    return datum.asFloat();
  } else if constexpr (std::is_same_v<T, double>) {
    return datum.asDouble();
  } else if constexpr (std::is_same_v<T, char>) {
    return datum.asString();
  } else {
    static_assert(!sizeof(T), "Invalid type to jsonValueToType");
  }
}

/// convert the metadata to a JSON representation compatible with the server
Json::Value modelMetadataToJson(const ModelMetadata &metadata);
ModelMetadata mapJsonToModelMetadata(const Json::Value *json);

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CLIENTS_HTTP_INTERNAL
