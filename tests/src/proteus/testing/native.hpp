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

#ifndef GUARD_SRC_PROTEUS_TESTING_NATIVE
#define GUARD_SRC_PROTEUS_TESTING_NATIVE

#include <string>  // for string

#include "proteus/clients/client.hpp"    // for Client
#include "proteus/core/predict_api.hpp"  // for InferenceRequest (ptr only)

namespace proteus {

class NativeClient : public Client {
 public:
  bool serverLive() override;
  bool serverReady() override;
  bool modelReady(const std::string& model) override;

  std::string modelLoad(const std::string& model,
                        RequestParameters* parameters) override;
  void modelUnload(const std::string& model) override;
  InferenceResponse modelInfer(const std::string& model,
                               const InferenceRequest& request) override;
};

#endif  // GUARD_SRC_PROTEUS_TESTING_NATIVE

}  // namespace proteus
