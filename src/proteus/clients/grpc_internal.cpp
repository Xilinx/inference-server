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
 * @brief Implements the internal objects used for gRPC
 */

#include "proteus/clients/grpc_internal.hpp"

#include <google/protobuf/repeated_ptr_field.h>  // for RepeatedPtrField

#include <cstddef>   // for size_t
#include <cstdint>   // for int16_t, int32_t
#include <iostream>  // for operator<<, cout
#include <memory>    // for make_shared, shared...
#include <vector>    // for vector, _Bit_reference

#include "predict_api.pb.h"                       // for ModelInferResponse_...
#include "proteus/core/data_types.hpp"            // for DataType, mapTypeToStr
#include "proteus/core/predict_api_internal.hpp"  // for RequestParameters
#include "proteus/helpers/declarations.hpp"       // for InferenceResponseOu...

namespace proteus {

void mapProtoToParameters(
  const google::protobuf::Map<std::string, inference::InferParameter>& params,
  RequestParameters* parameters) {
  using ParameterType = inference::InferParameter::ParameterChoiceCase;
  for (const auto& [key, value] : params) {
    auto type = value.parameter_choice_case();
    switch (type) {
      case ParameterType::kBoolParam: {
        parameters->put(key, value.bool_param());
        break;
      }
      case ParameterType::kInt64Param: {
        // TODO(varunsh): parameters should switch to uint64?
        parameters->put(key, static_cast<int>(value.int64_param()));
        break;
      }
      case ParameterType::kDoubleParam: {
        parameters->put(key, value.double_param());
        break;
      }
      case ParameterType::kStringParam: {
        parameters->put(key, value.string_param());
        break;
      }
      default: {
        // if not set
        break;
      }
    }
  }
}

RequestParametersPtr mapProtoToParameters(
  const google::protobuf::Map<std::string, inference::InferParameter>& params) {
  auto parameters = std::make_shared<RequestParameters>();
  mapProtoToParameters(params, parameters.get());

  return parameters;
}

void mapProtoToParameters(
  const google::protobuf::Map<std::string, inference::InferParameter>& params,
  RequestParameters& parameters) {
  mapProtoToParameters(params, &parameters);
}

void mapResponsetoProto(InferenceResponse response,
                        inference::ModelInferResponse& reply) {
  reply.set_model_name(response.getModel());
  reply.set_id(response.getID());
  auto outputs = response.getOutputs();
  for (InferenceResponseOutput& output : outputs) {
    auto* tensor = reply.add_outputs();
    tensor->set_name(output.getName());
    // auto* parameters = tensor->mutable_parameters();
    tensor->set_datatype(output.getDatatype().str());
    auto shape = output.getShape();
    auto size = 1U;
    for (const size_t& index : shape) {
      tensor->add_shape(index);
      size *= index;
    }

    const auto output_size = output.getSize();
    switch (output.getDatatype()) {
      case DataType::BOOL: {
        const auto* data = static_cast<bool*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_bool_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::UINT8: {
        const auto* data = static_cast<uint8_t*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_uint_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::UINT16: {
        const auto* data = static_cast<uint16_t*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_uint_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::UINT32: {
        const auto* data = static_cast<uint32_t*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_uint_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::UINT64: {
        const auto* data = static_cast<uint64_t*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_uint64_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::INT8: {
        const auto* data = static_cast<int8_t*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_int_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::INT16: {
        const auto* data = static_cast<int16_t*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_int_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::INT32: {
        const auto* data = static_cast<int32_t*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_int_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::INT64: {
        const auto* data = static_cast<int64_t*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_int64_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        std::cout << "Writing FP16 not supported\n";
        break;
      }
      case DataType::FP32: {
        const auto* data = static_cast<float*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_fp32_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::FP64: {
        const auto* data = static_cast<double*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_fp64_contents();
        for (size_t i = 0; i < output_size; i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::STRING: {
        const auto* data = static_cast<char*>(output.getData());
        auto* contents = tensor->mutable_contents()->mutable_bytes_contents();
        contents->Add(data);
        // for(size_t i = 0; i < output.getSize(); i++){
        //   contents->Add(data->data()[i]);
        // }
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }
  }
}

void mapModelMetadataToProto(const ModelMetadata& metadata,
                             inference::ModelMetadataResponse& resp) {
  resp.set_name(metadata.getName());
  resp.set_platform(metadata.getPlatform());

  const auto& inputs = metadata.getInputs();
  for (const auto& input : inputs) {
    auto* tensor = resp.add_inputs();
    tensor->set_name(input.getName());
    tensor->set_datatype(input.getDataType().str());
    const auto& shape = input.getShape();
    for (const auto& i : shape) {
      tensor->add_shape(i);
    }
  }
  const auto& outputs = metadata.getOutputs();
  for (const auto& output : outputs) {
    auto* tensor = resp.add_outputs();
    tensor->set_name(output.getName());
    tensor->set_datatype(output.getDataType().str());
    const auto& shape = output.getShape();
    for (const auto& i : shape) {
      tensor->add_shape(i);
    }
  }
}

}  // namespace proteus
