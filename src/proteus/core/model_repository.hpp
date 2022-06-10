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

#ifndef GUARD_PROTEUS_CORE_MODEL_REPOSITORY
#define GUARD_PROTEUS_CORE_MODEL_REPOSITORY

#include <filesystem>
#include <string>

namespace proteus {

class RequestParameters;

class ModelRepository {
 public:
  static void modelLoad(const std::string& model,
                        RequestParameters* parameters);

  static void setRepository(const std::string& repository);

 private:
  class ModelRepositoryImpl {
   public:
    void modelLoad(const std::string& model,
                   RequestParameters* parameters) const;

    void setRepository(const std::string& repository);

   private:
    std::filesystem::path repository_;
  };

  inline static ModelRepositoryImpl repo_;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_MODEL_REPOSITORY
