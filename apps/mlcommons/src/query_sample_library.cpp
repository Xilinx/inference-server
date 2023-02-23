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

#include "query_sample_library.hpp"

#include <cassert>

namespace fs = std::filesystem;

namespace amdinfer {

QuerySampleLibrary::QuerySampleLibrary(size_t perf_samples,
                                       const fs::path& directory,
                                       PreprocessFunc f)
  : perf_samples_(perf_samples), pre_process_(std::move(f)) {
  for (const auto& path : fs::recursive_directory_iterator(directory)) {
    if (!path.is_directory()) {
      auto sample_path = path.path();
      assert(fs::exists(sample_path));
      assert(fs::is_regular_file(sample_path));
      samples_.emplace_back(sample_path);
    }
  }
}

const std::string& QuerySampleLibrary::Name() const { return name_; }

size_t QuerySampleLibrary::TotalSampleCount() { return samples_.size(); }

size_t QuerySampleLibrary::PerformanceSampleCount() { return perf_samples_; }

void QuerySampleLibrary::LoadSamplesToRam(
  const std::vector<mlperf::QuerySampleIndex>& indices) {
  for (const auto& index : indices) {
    auto& sample = samples_[index];
    sample.request = pre_process_(sample.filepath);
  }
}

void QuerySampleLibrary::UnloadSamplesFromRam(
  const std::vector<mlperf::QuerySampleIndex>& indices) {
  for (const auto& index : indices) {
    samples_[index].request = InferenceRequest();
  }
}

InferenceRequest& QuerySampleLibrary::getSample(
  mlperf::QuerySampleIndex index) {
  return samples_[index].request;
}

}  // namespace amdinfer
