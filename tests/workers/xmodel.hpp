// Copyright 2021 Xilinx, Inc.
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

#ifndef GUARD_CPP_NATIVE_XMODEL
#define GUARD_CPP_NATIVE_XMODEL

#include <concurrentqueue/blockingconcurrentqueue.h>  // for BlockingConcurr...

#include <algorithm>               // for max, copy
#include <chrono>                  // for duration, opera...
#include <cstdint>                 // for uint64_t, uint32_t
#include <functional>              // for ref
#include <future>                  // for future, async
#include <iostream>                // for operator<<, bas...
#include <iterator>                // for back_insert_ite...
#include <memory>                  // for unique_ptr, mak...
#include <string>                  // for string, operator==
#include <utility>                 // for move, pair
#include <vart/runner.hpp>         // for Runner::TensorF...
#include <vart/runner_ext.hpp>     // for RunnerExt
#include <vector>                  // for vector
#include <xir/graph/graph.hpp>     // for Graph
#include <xir/graph/subgraph.hpp>  // for Subgraph
#include <xir/tensor/tensor.hpp>   // for Tensor

#include "amdinfer/amdinfer.hpp"
#include "amdinfer/core/data_types_internal.hpp"
#include "amdinfer/util/containers.hpp"

inline std::vector<const xir::Subgraph*> getDpuSubgraphs(xir::Graph* graph) {
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

inline void queueReference(int images, vart::Runner* runner,
                           PairQueue& my_queue) {
  auto* dpu_runner = dynamic_cast<vart::RunnerExt*>(runner);
  auto inputs = dpu_runner->get_inputs();
  auto outputs = dpu_runner->get_outputs();
  // std::pair<uint32_t, int> elem;
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

inline int runReference(const std::string& xmodel, int images, int threads,
                        int runners) {
  std::unique_ptr<xir::Graph> graph = xir::Graph::deserialize(xmodel);
  auto subgraphs = getDpuSubgraphs(graph.get());

  std::vector<std::unique_ptr<vart::Runner>> runners_ptrs;
  for (auto i = 0; i < runners; i++) {
    auto runner = vart::Runner::create_runner(subgraphs[0], "run");
    runners_ptrs.push_back(std::move(runner));
  }

  PairQueue my_queue;
  const int batch_size = 4;
  const int batches = images / batch_size;  // execute_async operates on batches

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
  futures.reserve(threads);
  for (int i = 0; i < threads; i++) {
    futures.push_back(
      std::async(std::launch::async, queueReference, batches / threads,
                 runners_ptrs[i % runners].get(), std::ref(my_queue)));
  }

  for (auto& future : futures) {
    future.get();
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
  auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(t1 - t0);

  std::cout << "Total Execution time for " << images
            << " queries: " << elapsed.count() << " ms" << std::endl;
  std::cout << "Average queries per second: " << images / elapsed_s.count()
            << " qps" << std::endl;

  return 0;
}

using FutureQueue =
  moodycamel::BlockingConcurrentQueue<std::future<amdinfer::InferenceResponse>>;

inline void enqueue(int images, const std::string& worker_name,
                    const amdinfer::InferenceRequest& request,
                    FutureQueue& my_queue) {
  amdinfer::NativeClient client;
  for (int i = 0; i < images; i++) {
    auto future = client.modelInferAsync(worker_name, request);
    my_queue.enqueue(std::move(future));
  }
}

inline void dequeue(int images, FutureQueue& my_queue) {
  std::future<amdinfer::InferenceResponse> element;
  for (auto i = 0; i < images; i++) {
    my_queue.wait_dequeue(element);
    auto results = element.get();
  }
}

inline int run(const std::string& xmodel, int images, int threads,
               int runners) {
  amdinfer::RequestParameters parameters;
  parameters.put("model", xmodel);
  parameters.put("share", false);
  auto threads_per_worker = std::max(threads / runners, 1);
  parameters.put("threads", threads_per_worker);
  parameters.put("batchers", 2);

  amdinfer::NativeClient client;
  auto worker_name = client.workerLoad("Xmodel", &parameters);
  for (auto i = 0; i < runners - 1; i++) {
    std::ignore = client.workerLoad("Xmodel", &parameters);
  }

  std::vector<uint64_t> shape;
  amdinfer::DataType type = amdinfer::DataType::Int8;
  {
    std::unique_ptr<xir::Graph> graph = xir::Graph::deserialize(xmodel);
    auto subgraphs = getDpuSubgraphs(graph.get());

    auto runner = vart::Runner::create_runner(subgraphs[0], "run");

    auto input_tensors = runner->get_input_tensors();
    auto input_shape = input_tensors[0]->get_shape();
    auto input_type = input_tensors[0]->get_data_type();

    std::copy(input_shape.begin() + 1, input_shape.end(),
              std::back_inserter(shape));
    type = amdinfer::mapXirToType(input_type);
  }

  std::vector<char> data;
  auto num_elements = amdinfer::util::containerProduct(shape);
  data.reserve(num_elements * type.size());

  FutureQueue my_queue;
  amdinfer::InferenceRequest request;
  request.addInputTensor(static_cast<void*>(data.data()), shape, type);

  std::vector<std::future<void>> futures;
  const int enqueue_threads = 1;
  futures.reserve(enqueue_threads * 2);
  auto t0 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < enqueue_threads; i++) {
    futures.emplace_back(std::async(std::launch::async, enqueue,
                                    images / enqueue_threads, worker_name,
                                    request, std::ref(my_queue)));
    futures.emplace_back(std::async(std::launch::async, dequeue,
                                    images / enqueue_threads,
                                    std::ref(my_queue)));
  }

  for (auto& future : futures) {
    future.get();
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(t1 - t0);
  auto elapsed_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);

  std::cout << "Total Execution time for " << images
            << " queries: " << elapsed_ms.count() << " ms" << std::endl;
  std::cout << "Average queries per second: " << images / elapsed.count()
            << " qps" << std::endl;

  return 0;
}

#endif  // GUARD_CPP_NATIVE_XMODEL
