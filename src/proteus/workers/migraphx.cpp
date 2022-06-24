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
 * @brief Implements the Migraphx worker.  
 */

#include <cstddef>  // for size_t, byte
#include <cstdint>  // for uint32_t, int32_t
#include <memory>   // for unique_ptr, allocator
#include <numeric>  // for accumulate
#include <string>   // for string
#include <thread>   // for thread
#include <utility>  // for move
#include <vector>   // for vector
#include <stdio.h>  // debug only

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

  // the model file to be loaded
  std::string input_file_;

  // Image properties are contained in the model
  unsigned output_classes_;
  unsigned image_width_, image_height_, image_channels_, image_size_;

  // Input / Output nodes
  std::string input_node_, output_node_;
  DataType input_dt_ = DataType::FP32;
  DataType output_dt_ = DataType::FP32;

};


std::thread MIGraphXWorker::spawn(BatchPtrQueue* input_queue) {
    std::cout << "MIGraphXWorker::spawn creating thread\n";
  return std::thread(&MIGraphXWorker::run, this, input_queue);
}

void MIGraphXWorker::doInit(RequestParameters* parameters){
    (void)parameters;  // suppress unused variable warning
    std::cout << "MIGraphXWorker::doInit\n";
  // debug: print the key-value pairs in parameters
    for (const auto& [k, v] : parameters->data()){
        std::cout << k << " :-----------------------------------------++++++++++++++++++++ -------------- ";
        std::visit([](const auto& x){ std::cout << x; }, v);
        std::cout << '\n';
    }
    if (parameters->has("model"))
        input_file_ = parameters->get<std::string>("model");
    else
        SPDLOG_LOGGER_ERROR(
        this->logger_,
        "MIGraphXWorker parameters required:  \"model\": \"<filepath>\"");  // Ideally exit since model not provided

    // trial and error: set this 
    this->batch_size_ = 10;

    // These values are set in the onnx model we're using.
    image_width_ = 224, image_height_ = 224, image_channels_ = 3;
    output_classes_ = 1000;
    std::cout << "end MIGraphXWorker::doInit\n";

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
  constexpr auto kBufferNum = 2U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         1 * this->batch_size_ * image_height_ * image_width_ * image_channels_, this->input_dt_);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         1 * this->batch_size_ * output_classes_, output_dt_);
  SPDLOG_LOGGER_INFO(this->logger_, std::string("MIGraphXWorker::doAllocate added ") + std::to_string(buffer_num) + " buffers");
  return buffer_num;
}

    // Load the model (Acquire)

    
    std::cout << "Acquiring model file " << input_file_ << std::endl;
    // Using parse_onnx() instead of load() because there's a bug at the time of writing
    this->prog_ = migraphx::parse_onnx(input_file_.c_str());
    std::cout << "Finished parsing ONNX model." << std::endl;
 
    //
    // Fetch the expected dimensions of the input from the parsed model
    //
    migraphx::program_parameter_shapes input_shapes = this->prog_.get_parameter_shapes();
    if(input_shapes.size() != 1){
      SPDLOG_LOGGER_ERROR(
        this->logger_, std::string("migraphx worker was passed a model with unexpected number of input shapes=") + std::to_string(2));
      return size_t(0);
    }


    migraphx::shape sh = input_shapes["data"];
    auto lenth = sh.lengths();    // vector of dimensions 1, 3, 224, 224
    if(lenth.size() != 4){
      SPDLOG_LOGGER_ERROR(
        this->logger_, std::string("migraphx worker was passed a model with unexpected number of input dimensions"));
        return size_t(0);
    }

    // todo:  convert migraphx enum for data types to inf. server's enum values so we can read data type from the model.
    // For now, hard-code to FP32
    // this->input_dt_ = this->input_dtype_map[sh.type()];

    // Compile step needs to annotate with batch size when saving compiled model (a new reqt.)
    // migraphx should be able to handle smaller batch, too.
    this->batch_size_ = 64;    // should match the migraphx batch size, fetched from the program.
                               // current workaround: first dimension of input tensor is batch size.

    // These values are set in the onnx model we're using.
    image_width_ = lenth[2], image_height_ = lenth[3], image_channels_ = lenth[1];
    // Fetch the expected output size (num of categories) from the parsed model.
    // For an output of 1000 label values, output_lengths should be a vector of {1, 1000}
	  std::vector<size_t> output_lengths = this->prog_.get_output_shapes()[0].lengths();
    output_classes_ =
              std::accumulate(output_lengths.begin(), output_lengths.end(), 1, std::multiplies<size_t>()); 

    // Allocate 
 
    constexpr auto kBufferNum = 2U;
    size_t buffer_num =
      static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
    // Allocate enough to hold 1 batch worth of images.  
    VectorBuffer::allocate(this->input_buffers_, buffer_num,
                          1 * this->batch_size_ * image_height_ * image_width_ * image_channels_, this->input_dt_);
    VectorBuffer::allocate(this->output_buffers_, buffer_num,
                          1 * this->batch_size_ * output_classes_, output_dt_);
    SPDLOG_LOGGER_INFO(this->logger_, std::string("MIGraphXWorker   init  Allocate added ") + std::to_string(buffer_num) + " buffers");

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

  return buffer_num;
}

void MIGraphXWorker::doAcquire(RequestParameters* parameters){
    std::cout << "MIGraphXWorker::doAcquire\n";
    (void) parameters;

//     // Load the model

//     if (parameters->has("model"))
//         input_file_ = parameters->get<std::string>("model");
//     else
//         SPDLOG_LOGGER_ERROR(
//         this->logger_,
//         "MIGraphXWorker parameters required:  \"model\": \"<filepath>\"");  // Ideally exit since model not provided
    
//     std::cout << "Acquiring model file " << input_file_ << std::endl;
//     // Using parse_onnx() instead of load() because there's a bug at the time of writing
//     this->prog_ = migraphx::parse_onnx(input_file_.c_str());
//     std::cout << "Finished parsing ONNX model." << std::endl;
//         // prog_.print();    

//     // Compile the model.  Hard-coded choices of offload_copy and gpu target.
//     migraphx::compile_options comp_opts;
//     comp_opts.set_offload_copy();
// #define GPU 1
//     std::string target_str;
// if(GPU)
//         target_str = "gpu";
//     else
//         target_str = "ref";
//     migraphx::target targ = migraphx::target(target_str.c_str());
//     std::cout << "compiling model...\n";

//     prog_.compile(migraphx::target("gpu"), comp_opts);    
//     std::cout << "done." << std::endl;
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

      // The initial implementation of migraphx worker assumes that the model has only one input tensor
      // (for Resnet models, an image) and identifying it by name is superfluous.
      auto inputs = req->getInputs();   // const std::vector<InferenceRequestInput>

      // We don't use the outputs portion of the request currently.  It is part of the kserve format
      // specification, which the Inference Server is intended to follow.  
      // "The $request_output JSON is used to request which output tensors should be returned from the model."
      // https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md
      //
      // Selecting the request output is only relevant to models that have more than one output tensor.
      //
      auto outputs = req->getOutputs();

      // for each input tensor (image).  This 
      for (unsigned int i = 0; i < inputs.size(); i++) {

          // bug: with multiple input requests, the different inputs all point to the same data buffer,
          // and the image is corrupted so that the bottom 3/4 of the image is flipped upside down.

        auto* input_buffer = inputs[i].getData();
        // void *pasdf = &(inputs[i]);
        // (void) pasdf;
        // comment out line 221 in predict_api.pp to make these values public:
        // void *iasdf = &(inputs[i].data_);
        // void *fasdf =  &(inputs[i].name_);
        // (void)iasdf ; (void) fasdf;

        int rows = inputs[i].getShape()[0];
        int cols = inputs[i].getShape()[1];
        SPDLOG_LOGGER_INFO(this->logger_, std::string("rows: ") + std::to_string(rows) + ", cols: " + std::to_string(cols) );

        // Output image version of the data for visual inspection--debugging only
        cv::Mat sample_img = cv::Mat(rows, cols, CV_32FC3, input_buffer)*255;
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

          // Transfer the migraphx results to output

          size_t result_size = migraphx_output.size();  // should be 1 item with 1000 (for resnet) categories
          if(result_size != 1)
            throw std::length_error("result count from migraphx call was not 1");

          // move from migraphx_output to outputs
          auto shape   = migraphx_output[0].get_shape();

          // recast the output from a blob to an array of float
          auto lengths = shape.lengths();
          size_t num_results =
              std::accumulate(lengths.begin(), lengths.end(), 1, std::multiplies<size_t>());
          float* results = reinterpret_cast<float*>(migraphx_output[0].data());
for(size_t ii = 0; ii < 3; ii++)
  printf("result %lu is %f\n",ii, results[ii]);

    // for debug only.  We return all results.  The worker does not interpret results.  Compare these with 
    //    values seen by client.
    float* myMax     = std::max_element(results, results + num_results);
    int answer     = myMax - results;

    std::cout << "the top-ranked index is " << answer << " val. " << *myMax << std::endl;

          // the kserve specification for response output is at 
          // https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md#response-output
          InferenceResponseOutput output;
          output.setDatatype(output_dt_);
          std::string output_name = outputs[i].getName();
          if (output_name.empty()) {
            output.setName(inputs[i].getName());
          } else {
            output.setName(output_name);
          }
          output.setShape({num_results});
          auto buffer = std::make_shared<std::vector<_Float32>>();
          buffer->resize(num_results);
          memcpy(&((*buffer)[0]), results, num_results * getSize(output_dt_));  //    <== extra copy from results to buffer to output?  output.setData(results)?
          auto my_data_cast = std::reinterpret_pointer_cast<std::byte>(buffer);
          output.setData(std::move(my_data_cast));

          void * asdf = output.getData();
          uint32_t* asdfg = static_cast<uint32_t*>( asdf);
          printf(" $$$ buffer[0] is %f and output starts with %X and type is %d\n", (*buffer)[0], *asdfg, (int)output.getDatatype());
          resp.addOutput(output);

        } catch (const std::exception& e) {
          SPDLOG_LOGGER_ERROR(this->logger_, e.what());
          req->runCallbackError("Something went wrong");
          continue;
        }
        SPDLOG_LOGGER_INFO(this->logger_, "finished migraphx eval");
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

void MIGraphXWorker::doRelease() {    std::cout << "MIGraphXWorker::doRelease\n";
}
void MIGraphXWorker::doDeallocate() {    std::cout << "MIGraphXWorker::doDeallocate\n";
}
void MIGraphXWorker::doDestroy() {    std::cout << "MIGraphXWorker::doDestroy\n";
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
