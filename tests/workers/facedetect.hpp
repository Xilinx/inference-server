// Copyright 2021 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
// Copyright 2022 Advanced Micro Devices Inc.
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

#ifndef GUARD_CPP_NATIVE_FACEDETECT
#define GUARD_CPP_NATIVE_FACEDETECT

#include <concurrentqueue/blockingconcurrentqueue.h>  // for BlockingConcurr...

#include <algorithm>              // for max
#include <cassert>                // for assert
#include <cstdint>                // for uint64_t
#include <cstring>                // for memcpy
#include <filesystem>             // for path, directory...
#include <functional>             // for ref
#include <future>                 // for future, __force...
#include <opencv2/core.hpp>       // for Mat, MatSize
#include <opencv2/imgcodecs.hpp>  // for imread
#include <queue>                  // for queue
#include <string>                 // for string, operator==
#include <utility>                // for move
#include <vector>                 // for vector

#include "amdinfer/amdinfer.hpp"  // for load, RequestPa...

namespace fs = std::filesystem;

using FutureQueue =
  moodycamel::BlockingConcurrentQueue<std::future<amdinfer::InferenceResponse>>;

// NOLINTNEXTLINE(misc-definitions-in-headers)
std::vector<char> img_data;

inline void enqueue(std::vector<std::string>& image_paths, int start_index,
                    int count, const std::string& worker_name,
                    FutureQueue& my_queue) {
  amdinfer::NativeClient client;
  for (int i = 0; i < count; i++) {
    auto img = cv::imread(image_paths[start_index]);
    auto shape = {static_cast<uint64_t>(img.size[0]),
                  static_cast<uint64_t>(img.size[1]),
                  static_cast<uint64_t>(img.channels())};
    auto size = img.size[0] * img.size[1] * 3;
    img_data.reserve(size);
    memcpy(img_data.data(), img.data, size);

    amdinfer::InferenceRequest request;
    request.addInputTensor(static_cast<void*>(img_data.data()), shape,
                           amdinfer::DataType::Uint8);

    auto future = client.modelInferAsync(worker_name, request);
    my_queue.enqueue(std::move(future));
  }
}

inline void run(std::vector<std::string> image_paths, int threads,
                const std::string& worker_name, FutureQueue& my_queue) {
  std::vector<std::future<void>> futures;
  auto images_per_thread = image_paths.size() / threads;

  for (int i = 0; i < threads; i++) {
    auto future = std::async(std::launch::async, enqueue, std::ref(image_paths),
                             i * images_per_thread, images_per_thread,
                             worker_name, std::ref(my_queue));
    (void)future;
  }
}

inline std::string load(int workers) {
  amdinfer::RequestParameters parameters;
  parameters.put("aks_graph_name", "facedetect");
  parameters.put("aks_graph",
                 "${AKS_ROOT}/graph_zoo/"
                 "graph_facedetect_u200_u250_amdinfer.json");
  parameters.put("share", false);

  amdinfer::NativeClient client;
  std::string endpoint;
  endpoint = client.workerLoad("AksDetect", &parameters);
  for (int i = 0; i < workers - 1; i++) {
    const auto new_endpoint = client.workerLoad("AksDetect", &parameters);
    assert(endpoint == new_endpoint);
  }
  return endpoint;
}

inline std::vector<std::string> getImages(const std::string& image_directory) {
  std::vector<std::string> images;

  std::queue<std::future<amdinfer::InferenceResponse>> my_queue;
  std::vector<std::string> image_paths;

  // Load Dataset
  for (const auto& p : fs::directory_iterator(image_directory)) {
    std::string file_extension = p.path().extension().string();
    if (file_extension == ".jpg" || file_extension == ".JPEG" ||
        file_extension == ".png") {
      image_paths.push_back(p.path().string());
    }
  }

  const int batch_size = 4;  // DPU batch size
  auto left_out = image_paths.size() % batch_size;
  if (left_out != 0) {  // Make a batch complete
    for (auto b = 0U; b < (batch_size - left_out); b++) {
      image_paths.push_back(image_paths[0]);
    }
  }

  return image_paths;
}

#endif  // GUARD_CPP_NATIVE_FACEDETECT
