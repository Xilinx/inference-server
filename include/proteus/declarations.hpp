// Copyright 2021 Xilinx Inc.
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

#ifndef GUARD_PROTEUS_DECLARATIONS
#define GUARD_PROTEUS_DECLARATIONS

#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace amdinfer {

class Buffer;

class Batch;

class Interface;
using InterfacePtr = std::unique_ptr<Interface>;
class WorkerInfo;

struct UpdateCommand;

class InferenceRequest;
class InferenceResponse;
class InferenceRequestInput;
using InferenceResponseOutput = InferenceRequestInput;

using BufferPtr = std::unique_ptr<Buffer>;
using BufferPtrs = std::vector<BufferPtr>;
using BufferRawPtrs = std::vector<Buffer*>;

using InferenceRequestPtr = std::shared_ptr<InferenceRequest>;

using InferenceResponseFuture = std::future<amdinfer::InferenceResponse>;

using StringMap = std::unordered_map<std::string, std::string>;

using Kernels = std::unordered_map<std::string, int>;

}  // namespace amdinfer

#endif  // GUARD_PROTEUS_DECLARATIONS
