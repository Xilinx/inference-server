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

/**
 * @file
 * @brief Benchmark for facedetect
 */

#include "facedetect.hpp"

#include <chrono>               // for duration, operator-, steady_clock
#include <cstdlib>              // for exit, getenv
#include <cxxopts/cxxopts.hpp>  // for value, OptionAdder, Options, OptionEx...
#include <iostream>             // for operator<<, basic_ostream, endl, cout

void dequeue(FutureQueue& my_queue, int num_images) {
  for (int i = 0; i < num_images; i++) {
    std::future<amdinfer::InferenceResponse> element;
    my_queue.wait_dequeue(element);
    auto results = element.get();
  }
}

int main(int argc, char* argv[]) {
  std::string path = std::string(std::getenv("PROTEUS_ROOT")) + "/tests/assets";
  int threads = 4;
  int runners = 4;
  int max_images = -1;

  try {
    cxxopts::Options options("test_facedetect",
                             "Test/benchmark the Facedetect model");
    options.add_options()(
      "p,path", "Path to directory containing at least one image to send",
      cxxopts::value<std::string>(path))(
      "t,threads", "Number of threads to use to enqueue/deque images",
      cxxopts::value<int>(threads))(
      "r,runners", "Number of runners (i.e. workers) to use in Proteus",
      cxxopts::value<int>(runners))(
      "i,images", "Maximum number of images to use from the path (-1 for all)",
      cxxopts::value<int>(max_images))("h,help", "Print help");

    auto result = options.parse(argc, argv);

    if (result.count("help") != 0U) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }
  } catch (const cxxopts::OptionException& e) {
    std::cout << "Error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  amdinfer::Server server;

  auto workerName = load(runners);
  auto image_paths = getImages(path);
  if (max_images > 0) {
    auto images = static_cast<size_t>(max_images);
    if (image_paths.size() >= images) {
      image_paths.resize(images);
    } else {
      auto last = image_paths.size();
      for (auto i = last; i < images; i++) {
        image_paths.push_back(image_paths[last]);
      }
    }
  }

  // calculate the rounded number of images enqueued
  auto num_images = (image_paths.size() / threads) * threads;

  auto t1 = std::chrono::steady_clock::now();

  FutureQueue my_queue;
  run(image_paths, threads, workerName, my_queue);

  // auto t2 = std::chrono::steady_clock::now();

  std::vector<std::future<void>> futures;
  futures.reserve(threads);
  for (int i = 0; i < threads; i++) {
    futures.push_back(std::async(std::launch::async, dequeue,
                                 std::ref(my_queue), num_images / threads));
  }
  for (auto& future : futures) {
    future.get();
  }
  // dequeue(my_queue, num_images);

  auto t3 = std::chrono::steady_clock::now();
  // auto run_time_taken = std::chrono::duration<double>(t2 - t1).count();
  // auto dequeue_time_taken = std::chrono::duration<double>(t3 - t2).count();
  auto time_taken = std::chrono::duration<double>(t3 - t1).count();
  auto throughput = static_cast<double>(num_images) / time_taken;

  // Print Stats
  // std::cout << "Run time: " << run_time_taken << std::endl;
  // std::cout << "dequeue time: " << dequeue_time_taken << std::endl;
  std::cout << "Total Execution time for " << num_images
            << " queries: " << time_taken * 1000 << " ms" << std::endl;
  std::cout << "Average queries per second: " << throughput << " qps"
            << std::endl;
}
