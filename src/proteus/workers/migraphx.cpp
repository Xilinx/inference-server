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

#include <stdio.h>  // debug only

#include <cstddef>  // for size_t, byte
#include <cstdint>  // for uint32_t, int32_t
#include <fstream>
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
#include "proteus/declarations.hpp"           // for BufferPtr, InferenceResp...
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_INFO, SPDL...
#include "proteus/observation/metrics.hpp"    // for Metrics
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "proteus/workers/worker.hpp"         // for Worker
#include "proteus_extensions/util/thread.hpp"  // for setThreadName

// opencv for debugging only --
#include <migraphx/filesystem.hpp>
#include <migraphx/migraphx.hpp>  // MIGraphX C++ API
#include <opencv2/core.hpp>       // for Mat, Vec3b, MatSize, Vec, CV_8SC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize

/**
 * @brief The Migraphx worker accepts the name of an migraphx model file as an
 * argument and compiles and evaluates it.
 *
 */

namespace proteus {

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

  // the model file to be loaded.  Supported types are *.onnx and *.mxr
  std::filesystem::path input_file_;

  // Image properties are contained in the model
  unsigned output_classes_;
  unsigned image_width_, image_height_, image_channels_, image_size_;

  // Input / Output nodes
  std::string input_node_, output_node_;
  DataType input_dt_;
  DataType output_dt_;

  // Enum-to-enum conversion to let us read data type from migraphx model.
  // The definitions are taken from the MIGraphX macro
  // MIGRAPHX_SHAPE_VISIT_TYPES

  DataType toDataType(migraphx_shape_datatype_t in) {
    switch (in) {
      // case 0 is tuple_type which we don't support here
      case migraphx_shape_bool_type:
        return DataType::BOOL;
      case migraphx_shape_half_type:
        return DataType::FP16;
      case migraphx_shape_float_type:
        return DataType::FP32;
      case migraphx_shape_double_type:
        return DataType::FP64;
      case migraphx_shape_uint8_type:
        return DataType::UINT8;
      case migraphx_shape_int8_type:
        return DataType::INT8;
      case migraphx_shape_uint16_type:
        return DataType::UINT16;
      case migraphx_shape_int16_type:
        return DataType::INT16;
      case migraphx_shape_int32_type:
        return DataType::INT32;
      case migraphx_shape_int64_type:
        return DataType::INT64;
      case migraphx_shape_uint32_type:
        return DataType::UINT32;
      case migraphx_shape_uint64_type:
        return DataType::UINT64;
      default:
        return DataType::UNKNOWN;
    }
  }
};

std::thread MIGraphXWorker::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&MIGraphXWorker::run, this, input_queue);
}

void MIGraphXWorker::doInit(RequestParameters* parameters) {
  // default batch size; client may request a change
  batch_size_ = 64;
#ifdef PROTEUS_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  PROTEUS_LOG_INFO(logger, " MIGraphXWorker::doInit \n");

  if (parameters->has("batch")) {
    this->batch_size_ = parameters->get<int>("batch");
  }
  if (parameters->has("model")) {
    input_file_ = parameters->get<std::string>("model");
  } else {
    PROTEUS_LOG_ERROR(
      logger, "MIGraphXWorker parameters required:  \"model\": \"<filepath>\"");
    // Throwing an exception causes server to delete this worker instance.
    // Client must try again.
    throw std::invalid_argument(
      "model file argument missing from model load request");
  }

  // Only load/compile the model once during the lifetime of the worker.
  // This worker does not deallocate or release resources until it's destroyed;
  // if you want to change them, request a new worker.
  //
  //                        Load the model.
  //
  std::filesystem::path filepath(this->input_file_);
  std::filesystem::path compiled_path(this->input_file_);
  std::filesystem::path onnx_path(this->input_file_);

  // Filename processing.
  // Take the root of the given model file name and look for either an *.mxr
  // or *.onnx extension (after loading and compiling an *.onnx file, this
  // worker saves it as an *.mxr file for future use)
  // A *.mxr file should also have its baked-in batch size
  // tacked onto its name, eg. resnet50-v2-7_b64.mxr
  compiled_path.replace_extension();
  compiled_path += (std::string("_b") + std::to_string(batch_size_));
  compiled_path.replace_extension(".mxr");

  onnx_path.replace_extension(".onnx");

  // Is there an mxr file?
  std::ifstream f(compiled_path.c_str());
  if (f.good()) {
    // Load the compiled MessagePack (*.mxr) file
    PROTEUS_LOG_INFO(
      logger, std::string("migraphx worker loading compiled model file ") +
                compiled_path.c_str());
    migraphx::file_options options;
    options.set_file_format("msgpack");

    // The hip library will throw a cryptic error if unable to connect with a
    // GPU at this point.
    try {
      this->prog_ = migraphx::load(compiled_path.c_str(), options);
    } catch (const std::exception& e) {
      std::string emsg = e.what();
      if (emsg.find("Failed to call function") != std::string::npos) {
        emsg = emsg + ".  Server could not connect to a GPU.";
      }
      PROTEUS_LOG_ERROR(logger, emsg);
      throw std::runtime_error(emsg);
      // prog_ does not need to be compiled.
    }
  } else {
    // Look for onnx file.  ifstream tests that the file can be opened
    f = std::ifstream(onnx_path.c_str());
    if (f.good()) {
      // Load the onnx file
      // Using parse_onnx() instead of load() because there's a bug at the
      // time of writing
      PROTEUS_LOG_INFO(logger,
                       std::string("migraphx worker loading ONNX model file ") +
                         onnx_path.c_str());

      migraphx::onnx_options onnx_opts;
      // onnx_opts.set_input_parameter_shape("data", {2, 3, 224, 224}); //
      // {todo:  read from user request here}  set_default_dim_value
      onnx_opts.set_default_dim_value(batch_size_);
      this->prog_ = migraphx::parse_onnx(onnx_path.c_str(), onnx_opts);

      auto param_shapes =
        prog_.get_parameter_shapes();  // program_parameter_shapes struct
      // auto input = param_shapes.names().front();  // "data", not currently
      // used

      // Compile the model.  Hard-coded choices of offload_copy and gpu
      // target.
      migraphx::compile_options comp_opts;
      comp_opts.set_offload_copy();

      // migraphx can support a reference (cpu) target as a fallback if GPU is
      // not found; not implemented here
#define GPU 1
      std::string target_str;
      if (GPU)
        target_str = "gpu";
      else
        target_str = "ref";
      migraphx::target targ = migraphx::target(target_str.c_str());
      // The hip library will throw a cryptic error if unable to connect with
      // a GPU at this point.
      try {
        prog_.compile(migraphx::target("gpu"), comp_opts);
      } catch (const std::exception& e) {
        std::string emsg = e.what();
        if (emsg.find("Failed to call function") != std::string::npos) {
          emsg = emsg + ".  Server could not connect to a GPU.";
        }
        PROTEUS_LOG_ERROR(logger, emsg);
        throw std::runtime_error(emsg);
      }

      // Save the compiled program as a MessagePack (*.mxr) file
      f = std::ifstream(compiled_path.c_str());
      if (!f.good()) {
        migraphx::file_options options;
        options.set_file_format("msgpack");

        migraphx::save(this->prog_, compiled_path.c_str(), options);
        PROTEUS_LOG_INFO(logger, std::string(" Saved compiled model file ") +
                                   compiled_path.c_str());
      }

    } else {
      // Not finding the model file makes it impossible to finish initializing
      // this worker
      PROTEUS_LOG_INFO(
        logger, std::string("migraphx worker cannot open the model file ") +
                  onnx_path.c_str() + " or " + compiled_path.c_str() +
                  ".  Does this path exist?");
      throw std::invalid_argument(std::string("model file ") +
                                  onnx_path.c_str() +
                                  " not found or can't be opened");
    }
  }
  //
  // Fetch the expected dimensions of the input from the parsed model.  At
  // this time we only support models with a single input tensor
  migraphx::program_parameter_shapes input_shapes =
    this->prog_.get_parameter_shapes();
  if (input_shapes.size() != 1) {
    PROTEUS_LOG_ERROR(logger,
                      std::string("migraphx worker was passed a model with "
                                  "unexpected number of input shapes=") +
                        std::to_string(input_shapes.size()));
    throw std::invalid_argument(
      std::string("migraphx worker was passed a model with unexpected number "
                  "of input shapes=") +
      std::to_string(input_shapes.size()));
  }

  migraphx::shape sh = input_shapes["data"];
  auto length =
    sh.lengths();  // For resnet50, a vector of dimensions 1, 3, 224, 224
  if (length.size() != 4) {
    PROTEUS_LOG_INFO(logger,
                     std::string("migraphx worker was passed a model with "
                                 "unexpected number of input dimensions=") +
                       std::to_string(length.size()));
    throw std::invalid_argument(
      std::string(("migraphx worker was passed a model with unexpected "
                   "number of input dimensions=") +
                  std::to_string(length.size())));
  }

  // Fetch the data types for input and output from the parsed/compiled model
  migraphx_shape_datatype_t input_type = sh.type();  // an enum
  this->input_dt_ = toDataType(input_type);
  migraphx::api::shapes output_shapes = prog_.get_output_shapes();
  migraphx_shape_datatype_t output_type = output_shapes[0].type();
  this->output_dt_ = toDataType(output_type);

  this->batch_size_ = length[0];

  // These values are set in the onnx model we're using.
  image_channels_ = length[1];
  image_width_ = length[2], image_height_ = length[3];
  image_size_ =
    image_width_ * image_height_ * image_channels_ * this->input_dt_.size();
  // Fetch the expected output size (num of categories) from the parsed model.
  // For an output of 1000 label values, output_lengths should be a vector of
  // {1, 1000}
  std::vector<size_t> output_lengths = output_shapes[0].lengths();
  output_classes_ = std::accumulate(
    output_lengths.begin(), output_lengths.end(), 1, std::multiplies<size_t>());
}

/**
 * @brief doAllocate() allocates a queue of buffers for input/output.
 * A functioning doAllocate() is required or else the worker will
 * simply fail to respond to post requests.
 * doAllocate() may be called multiple times by the engine
 *  to allocate more buffer space if necessary.
 *
 * @param num the number of buffers to add
 * @return size_t the number of buffers added
 */
size_t MIGraphXWorker::doAllocate(size_t num) {
#ifdef PROTEUS_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif
  //
  // Allocate
  //
  constexpr auto kBufferNum = 2U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  // Allocate enough to hold 1 batch worth of images.
  VectorBuffer::allocate(
    this->input_buffers_, buffer_num,
    1 * this->batch_size_ * image_height_ * image_width_ * image_channels_,
    this->input_dt_);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         1 * this->batch_size_ * output_classes_, output_dt_);
  PROTEUS_LOG_INFO(logger, std::string("MIGraphXWorker::doAllocate() added ") +
                             std::to_string(buffer_num) + " buffers");

  return buffer_num;
}

void MIGraphXWorker::doAcquire(RequestParameters* parameters) {
  (void)parameters;
}

void MIGraphXWorker::doRun(BatchPtrQueue* input_queue) {
#ifdef PROTEUS_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif
  PROTEUS_LOG_INFO(logger, "beginning of MIGraphXWorker::doRun");

  // std::shared_ptr<InferenceRequest> req;
  setThreadName("Migraphx");

  //
  //  Wait for requests from the batcher in an infinite loop.  This thread will
  // run, waiting for more input, until the server kills it.  If a bad request
  // causes an exception, the server will return a REST failure message to the
  // client and continue waiting for requests.
  //

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    PROTEUS_LOG_INFO(logger, "New batch request in migraphx");
    std::chrono::time_point batch_tp =
      std::chrono::high_resolution_clock::now();
#ifdef PROTEUS_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::kPipelineIngressWorker);
#endif

    // Safety check:  count all the items in all the requests in this batch
    size_t items_in_batch = batch->size();

    PROTEUS_LOG_DEBUG(logger,
                      "MIGraphXWorker::doRun received request batch with " +
                        std::to_string(batch->size()) + " requests, " +
                        std::to_string(items_in_batch) + " items");
    if (items_in_batch > this->batch_size_) {
      PROTEUS_LOG_WARN(logger, "migraphx was passed a batch with " +
                                 std::to_string(items_in_batch) +
                                 " items, batch size is only " +
                                 std::to_string(batch_size_) +
                                 ".  Discarding extras.");
    }

    //
    //             run evaluation on the entire batch
    //

    // The 0'th data pointer for the 0'th request is the base address of the
    // whole data array for the batch
    auto& req0 = batch->getRequest(0);

    // The initial implementation of migraphx worker assumes that the model
    // has only one input tensor (for Resnet models, an image) and identifying
    // it by name is superfluous.
    auto inputs0 = req0->getInputs();

    auto* input_buffer = inputs0[0].getData();

    // the next 3 values are part of the model and don't need to be read here
    // except for verification In the input buffer, the 0'th dimension is color
    // channels (3)
    // int rows = inputs0[0].getShape()[1];
    // int cols = inputs0[0].getShape()[2];

    // DataType dtype = inputs0[0].getDatatype();

    // The MIGraphX operation: run the migraphx eval() method.
    // If migraphx exceptions happen, they will be handled
    try {
      migraphx::program_parameters params;

      // populate the migraphx parameters with shape read from the onnx
      // model.
      auto param_shapes = prog_.get_parameter_shapes();

      // No validation; it should not be possible to receive an empty
      // param_shapes or names
      auto input = param_shapes.names().front();  // "data"

      params.add(input, migraphx::argument(param_shapes[input], input_buffer));
      //
      // Run the inference
      //
      PROTEUS_LOG_INFO(logger, "Beginning migraphx eval");
      std::chrono::time_point eval_tp =
        std::chrono::high_resolution_clock::now();
      migraphx::api::arguments migraphx_output = this->prog_.eval(params);
      auto eval_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now() - eval_tp);
      PROTEUS_LOG_INFO(
        logger, std::string("Finished migraphx eval; batch size: ") +
                  std::to_string(batch_size_) + "  elapsed time: " +
                  std::to_string(eval_duration.count()) + " us.  Images/sec: " +
                  std::to_string(1.e6 * batch_size_ / (eval_duration.count())));

      //
      //           Fetch the results and populate response to each request in
      //           the batch
      //

      // tracking the location of individual images in a request requires
      // counting how many were in all the previous requests
      size_t input_index = 0;

      // for each request in the batch
      for (unsigned int j = 0; j < batch->size(); j++) {
        auto& req = batch->getRequest(j);
        try {
          InferenceResponse resp;
          resp.setID(req->getID());
          resp.setModel("migraphx");

          // We don't use the outputs portion of the request currently.  It is
          // part of the kserve format specification, which the Inference Server
          // is intended to follow. "The $request_output JSON is used to request
          // which output tensors should be returned from the model."
          // https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md
          //
          // Selecting the request output is only relevant to models that have
          // more than one output tensor.
          //
          auto inputs =
            req0->getInputs();  // const std::vector<InferenceRequestInput>
          auto outputs =
            req->getOutputs();  // one result vector for each request
          //
          // Transfer the migraphx results to output
          //
          size_t result_size =
            migraphx_output
              .size();  // should be 1 item with items for each input, each
                        // input has 1000 (for resnet) categories
          if (result_size != 1)
            throw std::length_error(
              "result count from migraphx call was not 1");
          auto shape = migraphx_output[0].get_shape();

          // recast the migraphx output from a blob to an array of
          // this->output_dt_
          auto lengths = shape.lengths();
          size_t num_results = std::accumulate(
            lengths.begin() + 1, lengths.end(), 1, std::multiplies<size_t>());
          // size of each result array, bytes
          size_t size_of_result = num_results * output_dt_.size();
          // for each image in the request, there should be 1 vector of 1000
          // values
          PROTEUS_LOG_DEBUG(logger, std::string("Request with ") +
                                      std::to_string(req->getInputs().size()) +
                                      " images");

          for (size_t k = 0;
               k < req->getInputs().size() && input_index < batch_size_; k++) {
            char* results =
              migraphx_output[0].data() + (input_index++) * size_of_result;
#define NDEBUG
#ifndef NDEBUG
            {
              float* zresults = reinterpret_cast<float*>(results);

              // for debug  Compare this result with
              //    values seen by client to verify output packet is correct.
              float* myMax = std::max_element(zresults, zresults + num_results);
              int answer = myMax - zresults;
              std::cout << "Ok.  the top-ranked index is " << answer << " val. "
                        << *myMax << std::endl;
            }
#endif
            // the kserve specification for response output is at
            // https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md#response-output
            InferenceResponseOutput output;
            output.setDatatype(output_dt_);
            // todo:  use real indexes here
            std::string output_name = outputs[0].getName();
            if (output_name.empty()) {
              output.setName(inputs[0].getName());
            } else {
              output.setName(output_name);
            }
            output.setShape({num_results});
            output.setData(results);

            // Copy migraphx results to a buffer and add to output
            std::vector<std::byte> buffer;
            buffer.resize(size_of_result);
            memcpy(buffer.data(), results, size_of_result);
            output.setData(std::move(buffer));
            resp.addOutput(output);
          }

          // respond back to the client
          req->runCallbackOnce(resp);
#ifdef PROTEUS_ENABLE_METRICS
          Metrics::getInstance().incrementCounter(
            MetricCounterIDs::kPipelineEgressWorker);
          auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - batch->getTime(j));
          Metrics::getInstance().observeSummary(
            MetricSummaryIDs::kRequestLatency, duration.count());
#endif
        } catch (const std::exception& e) {
          PROTEUS_LOG_ERROR(logger, e.what());
          // Pass error message back as reply to request; continue processing
          // more inference requests

          req->runCallbackError(
            std::string("Error processing Migraphx request: ") + e.what());
        }
      }  // end j, request
    } catch (const std::exception& e) {
      // This outer catch block catches exceptions in evaluation of the batch.
      PROTEUS_LOG_ERROR(logger, e.what());
      // Pass error message back as reply for each request in the batch
      const auto& requests = batch->getRequests();
      for (auto& req_e : requests) {
        req_e->runCallbackError(std::string("Migraphx inference error: ") +
                                e.what());
      }
    }

    auto batch_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now() - batch_tp);
    PROTEUS_LOG_INFO(
      logger, std::string("Finished migraphx batch processing; batch size: ") +
                std::to_string(batch_size_) + "  elapsed time: " +
                std::to_string(batch_duration.count()) + " us");
  }  // end while (batch)
  PROTEUS_LOG_INFO(logger, "Migraphx::doRun ending");
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
