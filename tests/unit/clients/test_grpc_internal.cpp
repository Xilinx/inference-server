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

#include <cstdint>  // for uint16_t, int16_t, int32_t

#include "gtest/gtest.h"  // for EXPECT_EQ, FAIL, UnitTest
#include "predict_api.pb.h"
#include "proteus/clients/grpc_internal.hpp"  // for mapRequestToProto

namespace proteus {

const bool kBoolValue = true;
const uint8_t kUint8Value = 2;
const uint16_t kUint16Value = 3;
const uint32_t kUint32Value = 4;
const uint64_t kUint64Value = 5;
const int8_t kInt8Value = -1;
const int16_t kInt16Value = -2;
const int32_t kInt32Value = -3;
const int64_t kInt64Value = -4;
const fp16 kFp16Value{1.4f};
const float kFloatValue = 2.7f;
const double kDoubleValue = 3.6;
const char kCharValue = 'x';

struct assignData {
  template <typename T>
  void operator()(std::byte* data) const {
    auto* data_cast = reinterpret_cast<T*>(data);
    if constexpr (std::is_same_v<T, bool>) {
      data_cast[0] = kBoolValue;
    } else if constexpr (std::is_same_v<T, uint8_t>) {
      data_cast[0] = kUint8Value;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
      data_cast[0] = kUint16Value;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      data_cast[0] = kUint32Value;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      data_cast[0] = kUint64Value;
    } else if constexpr (std::is_same_v<T, int8_t>) {
      data_cast[0] = kInt8Value;
    } else if constexpr (std::is_same_v<T, int16_t>) {
      data_cast[0] = kInt16Value;
    } else if constexpr (std::is_same_v<T, int32_t>) {
      data_cast[0] = kInt32Value;
    } else if constexpr (std::is_same_v<T, int64_t>) {
      data_cast[0] = kInt64Value;
    } else if constexpr (std::is_same_v<T, fp16>) {
      data_cast[0] = kFp16Value;
    } else if constexpr (std::is_same_v<T, float>) {
      data_cast[0] = kFloatValue;
    } else if constexpr (std::is_same_v<T, double>) {
      data_cast[0] = kDoubleValue;
    } else if constexpr (std::is_same_v<T, char>) {
      data_cast[0] = kCharValue;
    } else {
      throw invalid_argument("Unsupported datatype to assignData");
    }
  }
};

struct checkData {
  template <typename T>
  void operator()(
    const inference::ModelInferRequest_InferInputTensor* tensor) const {
    auto* contents = getTensorContents<T>(tensor);
    if constexpr (std::is_same_v<T, bool>) {
      EXPECT_EQ(*contents, kBoolValue);
    } else if constexpr (std::is_same_v<T, uint8_t>) {
      EXPECT_EQ(*contents, kUint8Value);
    } else if constexpr (std::is_same_v<T, uint16_t>) {
      EXPECT_EQ(*contents, kUint16Value);
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      EXPECT_EQ(*contents, kUint32Value);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      EXPECT_EQ(*contents, kUint64Value);
    } else if constexpr (std::is_same_v<T, int8_t>) {
      EXPECT_EQ(*contents, kInt8Value);
    } else if constexpr (std::is_same_v<T, int16_t>) {
      EXPECT_EQ(*contents, kInt16Value);
    } else if constexpr (std::is_same_v<T, int32_t>) {
      EXPECT_EQ(*contents, kInt32Value);
    } else if constexpr (std::is_same_v<T, int64_t>) {
      EXPECT_EQ(*contents, kInt64Value);
    } else if constexpr (std::is_same_v<T, fp16>) {
      EXPECT_FLOAT_EQ(*contents, kFp16Value);
    } else if constexpr (std::is_same_v<T, float>) {
      EXPECT_FLOAT_EQ(*contents, kFloatValue);
    } else if constexpr (std::is_same_v<T, double>) {
      EXPECT_EQ(*contents, kDoubleValue);
    } else if constexpr (std::is_same_v<T, char>) {
      EXPECT_EQ(contents[0][0][0], kCharValue);
    } else {
      throw invalid_argument("Unsupported datatype to checkData");
    }
  }
};

class Fixture : public testing::TestWithParam<DataType> {};

TEST_P(Fixture, TestRequestToProto) {
  auto datatype = GetParam();

  // create an array large enough to hold any data type
  std::array<std::byte, sizeof(double)> data;

  InferenceRequest request;
  InferenceRequestInput input;
  input.setData(data.data());
  switchOverTypes(assignData(), datatype, data.data());
  input.setDatatype(datatype);
  request.addInputTensor(input);

  inference::ModelInferRequest proto_request;
  mapRequestToProto(request, proto_request);

  const auto& tensor = proto_request.inputs().at(0);
  switchOverTypes(checkData(), datatype, &tensor);
}

// we exclude STRING as it doesn't have a defined size we can pre-allocate
const std::array<DataType, 12> datatypes = {
  DataType::BOOL,   DataType::UINT8, DataType::UINT16, DataType::UINT32,
  DataType::UINT64, DataType::INT8,  DataType::INT16,  DataType::INT32,
  DataType::INT64,  DataType::FP16,  DataType::FP32,   DataType::FP64};
INSTANTIATE_TEST_SUITE_P(UnitClientsGrpcInternal, Fixture,
                         testing::ValuesIn(datatypes));

}  // namespace proteus
