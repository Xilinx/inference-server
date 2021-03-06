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
 * @brief Defines the client-server API used in the inference server
 */

#ifndef GUARD_PROTEUS_CORE_API
#define GUARD_PROTEUS_CORE_API

#include <memory>
#include <string>
#include <vector>

#include "proteus/helpers/declarations.hpp"

namespace proteus {

class ModelMetadata;
class RequestParameters;
class ServerMetadata;
class Interface;

void modelLoad(const std::string& model, RequestParameters* parameters);
void modelUnload(const std::string& model);
std::string workerLoad(const std::string& worker,
                       RequestParameters* parameters);
void workerUnload(const std::string& worker);

ServerMetadata serverMetadata();
std::vector<std::string> modelList();
bool modelReady(const std::string& model);
ModelMetadata modelMetadata(const std::string& model);

void modelInfer(const std::string& model, std::unique_ptr<Interface> request);

Kernels getHardware();
bool hasHardware(const std::string& name, int num);

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_API
