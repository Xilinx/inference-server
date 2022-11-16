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
 * @brief Defines the internal objects used for HTTP/REST
 */

#ifndef GUARD_PROTEUS_CLIENTS_HTTP_INTERNAL
#define GUARD_PROTEUS_CLIENTS_HTTP_INTERNAL

#include <drogon/HttpRequest.h>   // for HttpRequestPtr
#include <drogon/HttpResponse.h>  // for HttpResponsePtr
#include <json/value.h>           // for Value

#include <cstddef>     // for size_t
#include <exception>   // for invalid_argument
#include <functional>  // for function
#include <memory>      // for shared_ptr
#include <string>      // for string
#include <vector>      // for vector

#include "amdinfer/build_options.hpp"              // for PROTEUS_ENABLE_TRACING
#include "amdinfer/core/interface.hpp"             // for Interface
#include "amdinfer/core/predict_api_internal.hpp"  // for InferenceRequestBui...
#include "amdinfer/declarations.hpp"               // for BufferRawPtrs, Infe...

namespace amdinfer {

/**
 * @brief Convert JSON-styled parameters to Proteus's implementation
 *
 * @param parameters
 * @return RequestParametersPtr
 */
RequestParametersPtr mapJsonToParameters(Json::Value parameters);
Json::Value mapParametersToJson(RequestParameters *parameters);

InferenceResponse mapJsonToResponse(Json::Value *json);
Json::Value mapRequestToJson(const InferenceRequest &request);

// class InferenceRequestOutputBuilder {
//  public:
//   static InferenceRequestOutput fromJson(
//     std::shared_ptr<Json::Value> const &req);
// };

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

#ifdef PROTEUS_ENABLE_TRACING
void propagate(drogon::HttpResponse *resp, const StringMap &context);
#endif

using DrogonCallback = std::function<void(const drogon::HttpResponsePtr &)>;

/**
 * @brief The DrogonHttp Interface class encapsulates incoming requests from
 * Drogon's HTTP interface to the batcher.
 *
 */
class DrogonHttp : public Interface {
 public:
  /**
   * @brief Construct a new DrogonHttp object
   *
   * @param req
   * @param callback
   */
  DrogonHttp(const drogon::HttpRequestPtr &req, DrogonCallback callback);

  std::shared_ptr<InferenceRequest> getRequest(
    const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
    const BufferRawPtrs &output_buffers,
    std::vector<size_t> &output_offsets) override;

  size_t getInputSize() override;
  void errorHandler(const std::exception &e) override;

 private:
  drogon::HttpRequestPtr req_;
  DrogonCallback callback_;
  std::shared_ptr<Json::Value> json_;
};

drogon::HttpResponsePtr errorHttpResponse(const std::string &error,
                                          int status_code);

/// convert the metadata to a JSON representation compatible with the server
Json::Value ModelMetadataToJson(const ModelMetadata &metadata);
ModelMetadata mapJsonToModelMetadata(const Json::Value *json);

}  // namespace amdinfer

#endif  // GUARD_PROTEUS_CLIENTS_HTTP_INTERNAL
