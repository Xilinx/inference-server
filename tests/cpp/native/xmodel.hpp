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

#ifndef GUARD_CPP_NATIVE_XMODEL
#define GUARD_CPP_NATIVE_XMODEL

#include <concurrentqueue/blockingconcurrentqueue.h>  // for BlockingConcurr...

#include <chrono>                 // for duration, opera...
#include <cstdint>                // for uint64_t, uint32_t
#include <cstring>                // for memcpy
#include <filesystem>             // for path, directory...
#include <functional>             // for ref
#include <future>                 // for future, async
#include <iostream>               // for operator<<, bas...
#include <memory>                 // for unique_ptr, mak...
#include <numeric>                // for accumulate
#include <opencv2/core.hpp>       // for Mat, MatSize
#include <opencv2/imgcodecs.hpp>  // for imread
#include <string>                 // for string, operator==
#include <system_error>           // for system_error
#include <thread>                 // for thread
#include <utility>                // for move, pair
#include <vart/runner.hpp>        // for Runner::TensorF...
#include <vart/runner_ext.hpp>    // for RunnerExt
#include <vector>                 // for vector
#include <xir/graph/graph.hpp>    // for Graph

#include "proteus/proteus.hpp"

std::vector<const xir::Subgraph*> get_dpu_subgraphs(xir::Graph* graph) {
  std::vector<xir::Subgraph*> subgraphs =
    graph->get_root_subgraph()->children_topological_sort();
  auto dpu_graphs = std::vector<const xir::Subgraph*>();
  for (auto* c : subgraphs) {
    auto device = c->get_attr<std::string>("device");
    if (device == "DPU") {
      dpu_graphs.emplace_back(c);
    }
  }
  return dpu_graphs;
}

using PairQueue = moodycamel::BlockingConcurrentQueue<std::pair<uint32_t, int>>;

void queue_reference(int images, vart::Runner* runner, PairQueue& my_queue) {
  auto dpu_runner = dynamic_cast<vart::RunnerExt*>(runner);
  auto inputs = dpu_runner->get_inputs();
  auto outputs = dpu_runner->get_outputs();
  std::pair<uint32_t, int> elem;
  for (int i = 0; i < images; i++) {
    // for (auto input : inputs) {
    //   input->sync_for_write(0, input->get_tensor()->get_element_num() /
    //                              input->get_tensor()->get_shape()[0]);
    // }
    // my_queue.wait_dequeue(elem);
    auto ret = dpu_runner->execute_async(inputs, outputs);
    dpu_runner->wait(uint32_t(ret.first), -1);

    // for (auto output : outputs) {
    //   output->sync_for_read(0, output->get_tensor()->get_element_num() /
    //                              output->get_tensor()->get_shape()[0]);
    // }
  }
  (void)my_queue;
}

int run_reference(std::string xmodel, int images, int threads, int runners) {
  std::unique_ptr<xir::Graph> graph = xir::Graph::deserialize(xmodel);
  auto subgraphs = get_dpu_subgraphs(graph.get());

  std::vector<std::unique_ptr<vart::Runner>> runners_ptrs;
  for (auto i = 0; i < runners; i++) {
    auto runner = vart::Runner::create_runner(subgraphs[0], "run");
    runners_ptrs.push_back(std::move(runner));
  }

  PairQueue my_queue;
  const int kBatchSize = 4;
  const int batches = images / kBatchSize;  // execute_async operates on batches

  // auto input_tensors = runners_ptrs[0]->get_input_tensors();
  // auto output_tensors = runners_ptrs[0]->get_output_tensors();

  // std::vector<std::unique_ptr<vart::TensorBuffer>> buffers;
  // std::vector<std::unique_ptr<xir::Tensor>> tensors;
  // std::vector<std::vector<vart::TensorBuffer*>> inputs;
  // inputs.resize(threads);
  // for(int t = 0; t < threads; t++){
  //   for(const auto& tensor : input_tensors){
  //     auto new_tensor = xir::Tensor::create(tensor->get_name(),
  //     tensor->get_shape(), tensor->get_data_type());
  //     buffers.emplace_back(std::make_unique<vart::CpuFlatTensorBufferOwned>(new_tensor.get()));
  //     inputs[t].push_back(buffers.back().get());
  //     tensors.emplace_back(std::move(new_tensor));
  //   }
  // }

  // std::vector<std::vector<vart::TensorBuffer*>> outputs;
  // outputs.resize(threads);
  // for(int t = 0; t < threads; t++){
  //   for(const auto& tensor : output_tensors){
  //     auto new_tensor = xir::Tensor::create(tensor->get_name(),
  //     tensor->get_shape(), tensor->get_data_type());
  //     buffers.emplace_back(std::make_unique<vart::CpuFlatTensorBufferOwned>(new_tensor.get()));
  //     outputs[t].push_back(buffers.back().get());
  //     tensors.emplace_back(std::move(new_tensor));
  //   }
  // }

  auto t0 = std::chrono::high_resolution_clock::now();

  // std::pair<uint32_t, int> elem;
  // for (int i = 0; i < batches; i++) {
  //   my_queue.enqueue(elem);
  // }

  std::vector<std::future<void>> futures;
  for (int i = 0; i < threads; i++) {
    futures.push_back(
      std::async(std::launch::async, queue_reference, batches / threads,
                 runners_ptrs[i % runners].get(), std::ref(my_queue)));
  }

  for (auto& future : futures) {
    future.get();
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = t1 - t0;

  std::cout << "Total Execution time for " << images
            << " queries: " << elapsed.count() * 1000 << " ms" << std::endl;
  std::cout << "Average queries per second: " << images / elapsed.count()
            << " qps" << std::endl;

  return 0;
}

using FutureQueue =
  moodycamel::BlockingConcurrentQueue<std::future<proteus::InferenceResponse>>;

void enqueue(int images, const std::string& workerName,
             proteus::InferenceRequestInput request, FutureQueue& my_queue) {
  for (int i = 0; i < images; i++) {
    auto future = proteus::enqueue(workerName, request);
    my_queue.enqueue(std::move(future));
  }
}

void dequeue(int images, FutureQueue& my_queue) {
  std::future<proteus::InferenceResponse> element;
  for (auto i = 0; i < images; i++) {
    my_queue.wait_dequeue(element);
    auto results = element.get();
  }
}

int run(std::string xmodel, int images, int threads, int runners) {
  proteus::RequestParameters parameters;
  parameters.put("xmodel", xmodel);
  parameters.put("share", false);
  auto threads_per_worker = std::max(threads / runners, 1);
  parameters.put("threads", threads_per_worker);
  parameters.put("batchers", 2);

  auto workerName = proteus::load("Xmodel", &parameters);
  for (auto i = 0; i < runners - 1; i++) {
    proteus::load("Xmodel", &parameters);
  }

  std::vector<uint64_t> shape;
  proteus::types::DataType type = proteus::types::DataType::INT8;
  {
    std::unique_ptr<xir::Graph> graph = xir::Graph::deserialize(xmodel);
    auto subgraphs = get_dpu_subgraphs(graph.get());

    auto runner = vart::Runner::create_runner(subgraphs[0], "run");

    auto input_tensors = runner->get_input_tensors();
    auto input_shape = input_tensors[0]->get_shape();
    auto input_type = input_tensors[0]->get_data_type();

    std::copy(input_shape.begin() + 1, input_shape.end(),
              std::back_inserter(shape));
    type = proteus::types::mapXirType(input_type);
  }

  std::vector<char> data;
  auto num_elements =
    std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
  data.reserve(num_elements * proteus::types::getSize(type));

  FutureQueue my_queue;
  proteus::InferenceRequestInput request((void*)data.data(), shape, type);

  std::vector<std::future<void>> futures;
  const int enqueue_threads = 1;
  futures.reserve(enqueue_threads * 2);
  auto t0 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < enqueue_threads; i++) {
    futures.emplace_back(std::async(std::launch::async, enqueue,
                                    images / enqueue_threads, workerName,
                                    request, std::ref(my_queue)));
    futures.emplace_back(std::async(std::launch::async, dequeue,
                                    images / enqueue_threads,
                                    std::ref(my_queue)));
  }

  for (auto& future : futures) {
    future.get();
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = t1 - t0;

  std::cout << "Total Execution time for " << images
            << " queries: " << elapsed.count() * 1000 << " ms" << std::endl;
  std::cout << "Average queries per second: " << images / elapsed.count()
            << " qps" << std::endl;

  return 0;
}

#endif  // GUARD_CPP_NATIVE_XMODEL
