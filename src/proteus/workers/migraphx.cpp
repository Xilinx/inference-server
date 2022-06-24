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

/**
 * @file
 * @brief Implements the Migraphx worker
 */

#include <cstddef>  // for size_t, byte
#include <cstdint>  // for uint32_t, int32_t
#include <memory>   // for unique_ptr, allocator
#include <string>   // for string
#include <thread>   // for thread
#include <utility>  // for move
#include <vector>   // for vector

#include "proteus/batching/hard.hpp"          // for HardBatcher
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"        // for DataType, DataType::UINT32
#include "proteus/core/predict_api.hpp"       // for InferenceRequest, Infere...
#include "proteus/helpers/declarations.hpp"   // for BufferPtr, InferenceResp...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_INFO, SPDL...
#include "proteus/observation/metrics.hpp"    // for Metrics
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "proteus/workers/worker.hpp"         // for Worker

#include <migraphx/migraphx.hpp>              // MIGraphX C++ API


/**
 * @brief The Migraphx worker accepts the name of an migraphx model file as an
 * argument and compiles and evaluates it.
 *
 */

namespace proteus {

// std::string constructMessage(const std::string& key, const std::string& data) {
//   const std::string labels = R"([])";
//   return R"({"key": ")" + key + R"(", "data": {"img": ")" + data +
//          R"(", "labels": )" + labels + "}}";
// }

using types::DataType;

namespace workers {
    
class MIGraphXWorker : public Worker {
 public:
  using Worker::Worker;
  std::thread spawn(BatchPtrQueue* input_queue) override;
    migraphx::program prog_;

 private:
  void doInit(RequestParameters* parameters) override;
  size_t doAllocate(size_t num) override;
  void doAcquire(RequestParameters* parameters) override;
  void doRun(BatchPtrQueue* input_queue) override;
  void doRelease() override;
  void doDeallocate() override;
  void doDestroy() override;

  std::string input_file_;


  // Input / Output nodes
  std::string input_node_, output_node_;
  DataType input_dt_ = DataType::FP32;
};


std::thread MIGraphXWorker::spawn(BatchPtrQueue* input_queue) {
    std::cout << "MIGraphXWorker::spawn creating thread\n";
  return std::thread(&MIGraphXWorker::run, this, input_queue);
}

void MIGraphXWorker::doInit(RequestParameters* parameters){
    (void)parameters;  // suppress unused variable warning
    std::cout << "MIGraphXWorker::doInit\n";
}

size_t MIGraphXWorker::doAllocate(size_t num){
    return num;
}

void MIGraphXWorker::doAcquire(RequestParameters* parameters){
    std::cout << "MIGraphXWorker::doAcquire\n";
    // Load the model
    std::string input_file;
    if (parameters->has("model"))
        input_file = parameters->get<std::string>("model");
    else
        SPDLOG_LOGGER_ERROR(
        this->logger_,
        "MIGraphXWorker parameters required:  \"model\": \"<filepath>\"");  // Ideally exit since model not provided
    
    std::cout << "Acquiring model file " << input_file << std::endl;
    // Using parse_onnx() instead of load() because there's a bug at the time of writing
    this->prog_ = migraphx::parse_onnx(input_file.c_str());
}

void MIGraphXWorker::doRun(BatchPtrQueue* input_queue){

    // thread housekeeping, boilerplate code following tf_zendnn example
    std::unique_ptr<Batch> batch;
    setThreadName("Migraphx");

    while (true) {
        input_queue->wait_dequeue(batch);
        if (batch == nullptr) {
            break;
        }
        SPDLOG_LOGGER_DEBUG(this->logger_,
                            "Got request in MIGraphX. Size: " +
                            std::to_string(batch->requests->size()));
#ifdef PROTEUS_ENABLE_METRICS
        Metrics::getInstance().incrementCounter(
        MetricCounterIDs::kPipelineIngressWorker);
#endif
        for (unsigned int j = 0; j < batch->requests->size(); j++) {
            auto& req = batch->requests->at(j);
            (void)req;  // suppress unused variable warning
#ifdef PROTEUS_ENABLE_TRACING
            auto& trace = batch->traces.at(j);
            trace->startSpan("echo");
#endif
        }
        std::vector<InferenceResponse> responses;
        responses.reserve(batch->requests->size());

        std::cout << "Input Graph: " << std::endl;    
        prog_.print();
        std::cout << std::endl;

        // takes an argument of type migraphx::program_parameters ;
        // see ...AMDMIGraphX/examples/vision/cpp_mnist/mnist_inference.cpp for example of program_parameters
        auto result = prog_.eval({});

        // todo: reply like this.
        //       req->runCallbackOnce(resp);
        // Copy the output from the model to the response object

    }
}

void MIGraphXWorker::doRelease() {}
void MIGraphXWorker::doDeallocate() {}
void MIGraphXWorker::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::MIGraphXWorker("MIGraphX", "gpu");
}
}  // extern C