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

#include "proteus/testing/native.hpp"

#include <future>  // for future

#include "proteus/clients/native.hpp"  // for enqueue

namespace proteus {

bool NativeClient::serverLive() { return true; }

bool NativeClient::serverReady() { return true; }

bool NativeClient::modelReady(const std::string& model) {
  return proteus::modelReady(model);
}

std::string NativeClient::modelLoad(const std::string& model,
                                    RequestParameters* parameters) {
  return proteus::load(model, parameters);
}

void NativeClient::modelUnload(const std::string& model) {
  return proteus::unload(model);
}

InferenceResponse NativeClient::modelInfer(const std::string& model,
                                           const InferenceRequest& request) {
  auto future = proteus::enqueue(model, request);
  return future.get();
}

}  // namespace proteus
