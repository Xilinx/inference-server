// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief
 */

#ifndef GUARD_AMDINFER_CORE_SHARED_STATE
#define GUARD_AMDINFER_CORE_SHARED_STATE

#include <filesystem>  // for path
#include <memory>      // for unique_ptr
#include <string>      // for string
#include <vector>      // for vector

#include "amdinfer/core/endpoints.hpp"         // for Endpoints
#include "amdinfer/core/model_repository.hpp"  // for ModelRepository
#include "amdinfer/core/predict_api.hpp"       // for ModelMetadata, ServerM...
#include "amdinfer/declarations.hpp"           // for Kernels

namespace amdinfer {

class ProtocolWrapper;
class ParameterMap;

class SharedState {
 public:
  void modelLoad(const std::string& model, ParameterMap* parameters);
  void modelUnload(const std::string& model);
  std::string workerLoad(const std::string& worker, ParameterMap* parameters);
  void workerUnload(const std::string& worker);

  static ServerMetadata serverMetadata();
  std::vector<std::string> modelList();
  bool modelReady(const std::string& model);
  ModelMetadata modelMetadata(const std::string& model);

  void modelInfer(const std::string& model,
                  std::unique_ptr<ProtocolWrapper> request);

  static Kernels getHardware();
  static bool hasHardware(const std::string& name, int num);

  void setRepository(const std::filesystem::path& repository_path,
                     bool load_existing);
  void enableRepositoryMonitoring(bool use_polling);

 private:
  Endpoints endpoints_;
  ModelRepository repository_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_SHARED_STATE
