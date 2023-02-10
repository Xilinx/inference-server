// Copyright 2022 Xilinx, Inc.
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

#ifndef GUARD_AMDINFER_CORE_MODEL_REPOSITORY
#define GUARD_AMDINFER_CORE_MODEL_REPOSITORY

#include <efsw/efsw.hpp>  // for FileWatcher, Action, FileWatchListener, Wat...
#include <filesystem>     // for path
#include <memory>         // for unique_ptr
#include <string>         // for string

namespace amdinfer {

class Endpoints;
class ParameterMap;

class UpdateListener : public efsw::FileWatchListener {
 public:
  UpdateListener(const std::filesystem::path& repository, Endpoints* endpoints)
    : repository_(repository), endpoints_(endpoints) {}
  void handleFileAction(efsw::WatchID watch_id, const std::string& dir,
                        const std::string& filename, efsw::Action action,
                        std::string old_filename) override;

 private:
  std::filesystem::path repository_;
  Endpoints* endpoints_;
};

void parseModel(const std::filesystem::path& repository,
                const std::string& model, ParameterMap* parameters);

class ModelRepository {
 public:
  void setRepository(const std::filesystem::path& repository_path,
                     bool load_existing);
  std::string getRepository() const;
  void setEndpoints(Endpoints* endpoints);
  void enableMonitoring(bool use_polling);

 private:
  std::filesystem::path repository_;
  Endpoints* endpoints_;
  std::unique_ptr<efsw::FileWatcher> file_watcher_;
  std::unique_ptr<UpdateListener> listener_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MODEL_REPOSITORY
