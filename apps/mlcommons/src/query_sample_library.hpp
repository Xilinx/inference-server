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

#ifndef GUARD_MLCOMMONS_SRC_QUERY_SAMPLE_LIBRARY
#define GUARD_MLCOMMONS_SRC_QUERY_SAMPLE_LIBRARY

#include <filesystem>
#include <functional>
#include <string>

#include "amdinfer/amdinfer.hpp"
#include "mlcommons/loadgen/query_sample_library.h"

namespace amdinfer {

struct Sample {
  explicit Sample(const std::filesystem::path& path) : filepath(path){};

  std::filesystem::path filepath;
  InferenceRequest request;
};

using PreProcessFunc =
  std::function<InferenceRequest(const std::filesystem::path&)>;

class QuerySampleLibrary : public mlperf::QuerySampleLibrary {
 public:
  QuerySampleLibrary(size_t perf_samples,
                     const std::filesystem::path& directory,
                     const PreProcessFunc& f);

  /// Get the name for the object
  const std::string& Name() const override;

  /// Get the total number of samples
  size_t TotalSampleCount() override;

  /// Get the number of samples that are guaranteed to fit in memory
  size_t PerformanceSampleCount() override;

  /**
   * @brief Load the requested samples to memory. In non-MultiStream scenarios,
   * a previously loaded sample will not be loaded again
   *
   * @param indices sample indices to load
   */
  void LoadSamplesToRam(
    const std::vector<mlperf::QuerySampleIndex>& indices) override;

  /**
   * @brief Unload the requested samples from memory. In non-MultiStream
   * scenarios, a previously unloaded sample will not be unloaded again
   *
   * @param indices sample indices to unload
   */
  void UnloadSamplesFromRam(
    const std::vector<mlperf::QuerySampleIndex>& indices) override;

  InferenceRequest& getSample(mlperf::QuerySampleIndex index);

 private:
  std::string name_{"AMD Inference Server - native"};
  size_t perf_samples_;
  std::vector<Sample> samples_;
  PreProcessFunc pre_process_;
};

}  // namespace amdinfer

#endif  // GUARD_MLCOMMONS_SRC_QUERY_SAMPLE_LIBRARY
