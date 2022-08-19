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
 * @brief Defines the internal objects used for the C++ API
 */

#ifndef GUARD_PROTEUS_CLIENTS_NATIVE_INTERNAL
#define GUARD_PROTEUS_CLIENTS_NATIVE_INTERNAL

#include <cstddef>    // for size_t
#include <exception>  // for exception
#include <future>     // for promise
#include <memory>     // for shared_ptr
#include <vector>     // for vector

#include "proteus/core/interface.hpp"        // for Interface
#include "proteus/core/predict_api.hpp"      // for InferenceRequest, Infere...
#include "proteus/helpers/declarations.hpp"  // for BufferRawPtrs

namespace proteus {

class CppNativeApi : public Interface {
 public:
  explicit CppNativeApi(InferenceRequest request);

  std::shared_ptr<InferenceRequest> getRequest(
    const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
    const BufferRawPtrs &output_buffers,
    std::vector<size_t> &output_offsets) override;

  size_t getInputSize() override;
  void errorHandler(const std::exception &e) override;
  std::promise<proteus::InferenceResponse> *getPromise();

 private:
  InferenceRequest request_;
  InferenceResponsePromisePtr promise_;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CLIENTS_NATIVE_INTERNAL
