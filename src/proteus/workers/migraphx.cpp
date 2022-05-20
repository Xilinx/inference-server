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
#include <numeric>  // for accumulate
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

// opencv for debugging only -- 
#include <opencv2/core.hpp>       // for Mat, Vec3b, MatSize, Vec, CV_8SC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize

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

    // trial and error: set this to the number of bytes in an image (specified for this model)
    this->batch_size_ = 3*224*224;
}

/**
 * @brief doAllocate() allocates a queue of buffers for input/output.  
 * A functioning doAllocate() is required or else the worker will 
 * simply fail to respond to post requests.
 * 
 * @param num the number of buffers to add 
 * @return size_t the number of buffers added
 */
size_t MIGraphXWorker::doAllocate(size_t num){
  std::cout << "MIGraphXWorker::doAllocate\n";
  constexpr auto kBufferNum = 1U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         1 * this->batch_size_, DataType::UINT32);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         1 * this->batch_size_, DataType::FP32);
  SPDLOG_LOGGER_INFO(this->logger_, std::string("MIGraphXWorker::doAllocate added ") + std::to_string(buffer_num) + " buffers");
  return buffer_num;
}

void MIGraphXWorker::doAcquire(RequestParameters* parameters){
    std::cout << "MIGraphXWorker::doAcquire\n";
    // Load the model
    // std::string input_file;
    if (parameters->has("model"))
        input_file_ = parameters->get<std::string>("model");
    else
        SPDLOG_LOGGER_ERROR(
        this->logger_,
        "MIGraphXWorker parameters required:  \"model\": \"<filepath>\"");  // Ideally exit since model not provided
    
    std::cout << "Acquiring model file " << input_file_ << std::endl;
    // Using parse_onnx() instead of load() because there's a bug at the time of writing
    this->prog_ = migraphx::parse_onnx(input_file_.c_str());
    std::cout << "Finished parsing ONNX model." << std::endl;
        // prog_.print();    

    // Compile the model.  Hard-coded choices of offload_copy and gpu target.
    migraphx::compile_options comp_opts;
    comp_opts.set_offload_copy();
#define GPU 1
    std::string target_str;
if(GPU)
        target_str = "gpu";
    else
        target_str = "ref";
    migraphx::target targ = migraphx::target(target_str.c_str());
    std::cout << "compiling model...\n";

    prog_.compile(migraphx::target("gpu"), comp_opts);    
    std::cout << "done." << std::endl;
}

void MIGraphXWorker::doRun(BatchPtrQueue* input_queue){
      SPDLOG_LOGGER_INFO(this->logger_, "beginning of doRun migraphx");
std::shared_ptr<InferenceRequest> req;
  setThreadName("Migraphx");

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    SPDLOG_LOGGER_INFO(this->logger_, "Got request in migraphx");
#ifdef PROTEUS_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::kPipelineIngressWorker);
#endif
    for (unsigned int j = 0; j < batch->requests->size(); j++) {
      auto& req = batch->requests->at(j);
#ifdef PROTEUS_ENABLE_TRACING
      auto& trace = batch->traces.at(j);
      trace->startSpan("migraphx");
#endif
      InferenceResponse resp;
      resp.setID(req->getID());
      resp.setModel("migraphx");
      auto inputs = req->getInputs();   // const std::vector<InferenceRequestInput>
      auto outputs = req->getOutputs();
      for (unsigned int i = 0; i < inputs.size(); i++) {
        auto* input_buffer = inputs[i].getData();
        // std::byte* output_buffer = outputs[i].getData(); //brian:  is this right?
        // auto* input_buffer = dynamic_cast<VectorBuffer*>(input_ptr);
        // auto* output_buffer = dynamic_cast<VectorBuffer*>(output_ptr);

        // uint32_t value = *static_cast<uint32_t*>(input_buffer);
        int rows = inputs[i].getShape()[0];
        int cols = inputs[i].getShape()[1];
        SPDLOG_LOGGER_INFO(this->logger_, std::string("rows: ") + std::to_string(rows) + ", cols: " + std::to_string(cols) );

        // Output image version of the data for visual inspection--debugging only
        cv::Mat sample_img = cv::Mat(rows, cols, CV_8UC3, input_buffer);
bool check = imwrite((std::string("sampleImage") + std::to_string(i) + ".jpg").c_str(), sample_img);
(void)check;
        std::cout <<"################## input is " << inputs[i] ;

        // this is my operation: run the migraphx eval() method.
        // If exceptions can happen, they should be handled
        try {
          migraphx::program_parameters params;

          // populate the migraphx parameters with shape read from the onnx model.
          auto param_shapes = prog_.get_parameter_shapes();  // program_parameter_shapes struct

          auto input        = param_shapes.names().front();  // "data"
          params.add(input, migraphx::argument(param_shapes[input], (void *)input_buffer));

          // Run the inference
          SPDLOG_LOGGER_INFO(this->logger_, "beginning migraphx eval");
          migraphx::api::arguments  migraphx_output =      this->prog_.eval(params);
          SPDLOG_LOGGER_INFO(this->logger_, "finishing migraphx eval");

          // parsing of results is copied from examples/vision/cpp_mnist/mnist_inference.cpp
          // not sure if relevant
          auto shape   = migraphx_output[0].get_shape();
            std::cout << "the shape of the output returned by migraphx is " ;
          for(int i: shape.lengths())
            std::cout << i << " x " << std::endl;
          // recast the output from a blob to an array of float
          auto lengths = shape.lengths();
          auto num_results =
              std::accumulate(lengths.begin(), lengths.end(), 1, std::multiplies<size_t>());
          float* results = reinterpret_cast<float*>(migraphx_output[0].data());
          // get the index of best matching result
          float* max     = std::max_element(results, results + num_results);
          int answer     = max - results;
          std::cout << "================ best match is " << std::to_string(*max) << std::endl;

          // Move the result from migraphx output to REST output

        InferenceResponseOutput output;
        output.setDatatype(types::DataType::UINT32);
        std::string output_name = outputs[i].getName();
        if (output_name.empty()) {
          output.setName(inputs[i].getName());
        } else {
          output.setName(output_name);
        }
        output.setShape({1});
        auto buffer = std::make_shared<std::vector<uint32_t>>();
        buffer->resize(1);
        (*buffer)[0] = answer;
        auto my_data_cast = std::reinterpret_pointer_cast<std::byte>(buffer);
        output.setData(std::move(my_data_cast));
        resp.addOutput(output);

        } catch (const std::exception& e) {
          SPDLOG_LOGGER_ERROR(this->logger_, e.what());
          req->runCallbackError("Something went wrong");
          continue;
        }
        SPDLOG_LOGGER_INFO(this->logger_, "finished migraphx eval");

        // output_buffer->write(value);
      }

#ifdef PROTEUS_ENABLE_TRACING
      auto context = trace->propagate();
      resp.setContext(std::move(context));
#endif

      // respond back to the client
      req->runCallbackOnce(resp);
#ifdef PROTEUS_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::kPipelineEgressWorker);
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - batch->start_times[j]);
      Metrics::getInstance().observeSummary(MetricSummaryIDs::kRequestLatency,
                                            duration.count());
#endif
    }

    // todo:  verify the size of input/output buffers and whether we allocated the right size
    this->returnBuffers(std::move(batch->input_buffers),
                        std::move(batch->output_buffers));
    SPDLOG_LOGGER_DEBUG(this->logger_, "Returned buffers");
  }
  SPDLOG_LOGGER_INFO(this->logger_, "Migraphx ending");
}

void MIGraphXWorker::doRelease() {    std::cout << "RELEASE." << std::endl;
}
void MIGraphXWorker::doDeallocate() {    std::cout << "DEALLOCATE" << std::endl;
}
void MIGraphXWorker::doDestroy() {    std::cout << "DESTROY" << std::endl;
}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::MIGraphXWorker("MIGraphX", "gpu");
}
}  // extern C