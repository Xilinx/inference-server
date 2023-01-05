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

#include <migraphx/migraphx.h>  // for migraphx_shape_datatype_t

#include <algorithm>              // for max
#include <cstddef>                // for byte, size_t
#include <cstring>                // for memcpy
#include <exception>              // for exception
#include <filesystem>             // for path
#include <fstream>                // for ifstream, operator<<
#include <functional>             // for multiplies
#include <map>                    // for map
#include <memory>                 // for allocator, unique_ptr
#include <migraphx/migraphx.hpp>  // for shape, program, progra...
#include <numeric>                // for accumulate
#include <ratio>                  // for micro
#include <stdexcept>              // for invalid_argument, runt...
#include <string>                 // for string, operator+, to_...
#include <thread>                 // for thread
#include <utility>                // for move
#include <vector>                 // for vector

#include "amdinfer/batching/hard.hpp"          // for BatchPtr, Batch, Batch...
#include "amdinfer/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/core/data_types.hpp"        // for DataType, operator<<
#include "amdinfer/core/exceptions.hpp"        // for invalid_argument, runt...
#include "amdinfer/core/predict_api.hpp"       // for InferenceRequest, Infe...
#include "amdinfer/declarations.hpp"           // for InferenceResponseOutput
#include "amdinfer/observation/logging.hpp"    // for AMDINFER_LOG_INFO, AMD...
#include "amdinfer/observation/metrics.hpp"    // for Metrics, MetricCounterIDs
#include "amdinfer/util/queue.hpp"             // for BufferPtrsQueue
#include "amdinfer/util/thread.hpp"            // for setThreadName
#include "amdinfer/util/timer.hpp"             // for Timer
#include "amdinfer/workers/worker.hpp"         // for Worker, kNumBufferAuto

namespace amdinfer::workers {

/**
 * @brief The Migraphx worker accepts the name of an migraphx model file as an
 * argument and compiles and evaluates it.
 *
 */
class MIGraphXWorker : public Worker {
 public:
  using Worker::Worker;
  std::thread spawn(BatchPtrQueue* input_queue) override;

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
  // The prog_ is populated by reading the model file and contains most of
  // the worker's important info such as number, data types and sizes of
  // input and output buffers
  migraphx::program prog_;

  // flag to pad out a batch with dummy data.  Sending a batch of requests
  // with uninitialized data may crash migraphx, for certain models.
  // If pad_batch_ is true, this worker will pad any unused request slots
  // in a batch with dummy copies of the first request.
  bool pad_batch_ = true;
  // Calculated sizes in bytes for each input tensor, by input name
  std::map<std::string, size_t> input_sizes_;
};

std::thread MIGraphXWorker::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&MIGraphXWorker::run, this, input_queue);
}

// Enum-to-enum conversion to let us read data type from migraphx model.
// The definitions are taken from the MIGraphX macro
// MIGRAPHX_SHAPE_VISIT_TYPES
DataType toDataType(migraphx_shape_datatype_t in) {
  switch (in) {
    // case 0 is tuple_type which we don't support here
    case migraphx_shape_bool_type:
      return DataType::Bool;
    case migraphx_shape_half_type:
      return DataType::Fp16;
    case migraphx_shape_float_type:
      return DataType::Fp32;
    case migraphx_shape_double_type:
      return DataType::Fp64;
    case migraphx_shape_uint8_type:
      return DataType::Uint8;
    case migraphx_shape_int8_type:
      return DataType::Int8;
    case migraphx_shape_uint16_type:
      return DataType::Uint16;
    case migraphx_shape_int16_type:
      return DataType::Int16;
    case migraphx_shape_int32_type:
      return DataType::Int32;
    case migraphx_shape_int64_type:
      return DataType::Int64;
    case migraphx_shape_uint32_type:
      return DataType::Uint32;
    case migraphx_shape_uint64_type:
      return DataType::Uint64;
    default:
      return DataType::Unknown;
  }
}

void MIGraphXWorker::doInit(RequestParameters* parameters) {
  // default batch size; client may request a change. Arbitrarily set to 64
  const int default_batch_size = 64;

  batch_size_ = default_batch_size;
  pad_batch_ = true;
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif
  // stringstream used for formatting logger messages
  std::string msg;
  std::stringstream smsg(msg);

  AMDINFER_LOG_INFO(logger, " MIGraphXWorker::doInit \n");

  if (parameters->has("batch")) {
    this->batch_size_ = parameters->get<int>("batch");
  }
  if (parameters->has("model")) {
    input_file_ = parameters->get<std::string>("model");
  } else {
    AMDINFER_LOG_ERROR(
      logger, "MIGraphXWorker parameters required:  \"model\": \"<filepath>\"");
    // Throwing an exception causes server to delete this worker instance.
    // Client must try again.
    throw std::invalid_argument(
      "model file argument missing from model load request");
  }
  if (parameters->has("pad_batch")) {
    this->pad_batch_ = parameters->get<bool>("pad_batch");
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
    AMDINFER_LOG_INFO(
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
      AMDINFER_LOG_ERROR(logger, emsg);
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
      AMDINFER_LOG_INFO(
        logger, std::string("migraphx worker loading ONNX model file ") +
                  onnx_path.c_str());

      migraphx::onnx_options onnx_opts;
      onnx_opts.set_default_dim_value(batch_size_);
      this->prog_ = migraphx::parse_onnx(onnx_path.c_str(), onnx_opts);

      auto param_shapes =
        prog_.get_parameter_shapes();  // program_parameter_shapes struct

      AMDINFER_LOG_INFO(logger,
                        std::string("migraphx worker loaded ONNX model file ") +
                          onnx_path.c_str());

      // Compile the model.  Hard-coded choices of offload_copy and gpu
      // target.
      migraphx::compile_options comp_opts;
      comp_opts.set_offload_copy();

      // migraphx can support a reference (cpu) target as a fallback if GPU is
      // not found; not implemented here
      const bool use_gpu = true;
      std::string target_str = use_gpu ? "gpu" : "ref";
      migraphx::target targ{target_str.c_str()};
      // The hip library will throw a cryptic error if unable to connect with
      // a GPU at this point.
      try {
        prog_.compile(migraphx::target("gpu"), comp_opts);
      } catch (const std::exception& e) {
        std::string emsg = e.what();
        if (emsg.find("Failed to call function") != std::string::npos) {
          emsg = emsg + ".  Server could not connect to a GPU.";
        }
        AMDINFER_LOG_ERROR(logger, emsg);
        throw std::runtime_error(emsg);
      }

      // Save the compiled program as a MessagePack (*.mxr) file
      f = std::ifstream(compiled_path.c_str());
      if (!f.good()) {
        migraphx::file_options options;
        options.set_file_format("msgpack");

        migraphx::save(this->prog_, compiled_path.c_str(), options);
        AMDINFER_LOG_INFO(logger, std::string(" Saved compiled model file ") +
                                    compiled_path.c_str());
      }

    } else {
      // Not finding the model file makes it impossible to finish initializing
      // this worker
      AMDINFER_LOG_INFO(
        logger, std::string("migraphx worker cannot open the model file ") +
                  onnx_path.c_str() + " or " + compiled_path.c_str() +
                  ".  Does this path exist?");
      throw std::invalid_argument(std::string("model file ") +
                                  onnx_path.c_str() +
                                  " not found or can't be opened");
    }
  }
  //
  // Fetch the expected dimensions of the input from the parsed model.
  migraphx::program_parameter_shapes input_shapes =
    this->prog_.get_parameter_shapes();
  const auto* input_name = input_shapes.names()[0];
  auto sh = input_shapes[input_name];
  auto length = sh.lengths();
  migraphx::api::shapes output_shapes = prog_.get_output_shapes();
  this->batch_size_ = length[0];
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
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif
  AMDINFER_LOG_INFO(logger, "MIGraphXWorker::doAllocate");
  //
  // Allocate
  //
  constexpr auto kBufferNum = 3U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  // Allocate enough to hold buffer_num batches' worth of input sets.
  // Extra batches allow server to hold more requests at one time
  // todo:  this try/catch was observed to just get stuck when batch size
  // is too big (approx. 56 for Yolov4 model); how to catch the error?

  // Calculate the total number of bytes required for all inputs

  BufferPtrs buffer_vec;

  migraphx::program_parameter_shapes input_shapes =
    this->prog_.get_parameter_shapes();

  // Work out the max. size of any input buffer, in bytes.  We'll allocate all
  // of them the same size in case a request puts them in mixed-up order.
  size_t max_buffer(0);
  for (const auto* aname : input_shapes.names()) {
    migraphx::shape ashape = input_shapes[aname];
    auto llen = ashape.lengths();
    // size of the buffer needed for this input
    auto asize = ashape.bytes();
    // size of a single request input (divide by batch size)
    input_sizes_[aname] = asize / *(ashape.lengths().begin());
    max_buffer = std::max(max_buffer, asize);
  }

  // Now, allocate the input and output buffers.

  try {
    for (const auto* aname : input_shapes.names()) {
      auto ashape = input_shapes[aname];
      auto llen = ashape.lengths();

      // todo: test whether VectorBuffer::allocate() does this in the right
      // order for multiple (kBufferNum) sets of buffers. It wasn't designed to
      // be called in a loop like this.  Using 1 in place of kBufferNum

      buffer_vec.emplace_back(
        std::make_unique<VectorBuffer>(max_buffer, DataType::Uint8));
    }
    this->input_buffers_->enqueue(std::move(buffer_vec));

    // Calculate max. output buffer size
    size_t out_buffer_size{0};
    migraphx::shapes output_shapes = this->prog_.get_output_shapes();
    for (const auto& ash : output_shapes) {
      out_buffer_size = std::max(out_buffer_size, ash.bytes());
    }

    // Output buffers aren't used by the engine at time of writing this,
    // but allocate them anyway. (Use number of outputs for kBufferNum)
    VectorBuffer::allocate(this->output_buffers_, output_shapes.size(),
                           out_buffer_size, amdinfer::DataType::Int8);
  } catch (...) {
    AMDINFER_LOG_ERROR(
      logger,
      std::string("MIGraphXWorker couldn't allocate buffer (batch size ") +
        std::to_string(batch_size_) + ")");
    throw runtime_error{"MIGraphXWorker couldn't allocate buffer"};
  }
  AMDINFER_LOG_INFO(logger, std::string("MIGraphXWorker::doAllocate() added ") +
                              std::to_string(buffer_num) + " buffers");

  return buffer_num;
}

void MIGraphXWorker::doAcquire(RequestParameters* parameters) {
  (void)parameters;
}

void MIGraphXWorker::doRun(BatchPtrQueue* input_queue) {
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif
  AMDINFER_LOG_INFO(logger, "beginning of MIGraphXWorker::doRun");

  util::setThreadName("Migraphx");

  // stringstream used for formatting logger messages
  std::string msg;
  std::stringstream smsg(msg);

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
    AMDINFER_LOG_INFO(logger, "New batch request in migraphx");
    util::Timer timer;
    timer.add("batch_start");
#ifdef AMDINFER_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::PipelineIngressWorker);
#endif

    // The MIGraphX operation: run the migraphx eval() method.
    // If migraphx exceptions happen, they will be handled

    // We only need to look at the 0'th request to set up evaluation, because
    // its input pointers (one for each input) are the base addresses of the
    // data for the entire batch. The different input tensors are not required
    // to be contiguous with each other.
    const auto& req0 = batch->getRequest(0);
    auto inputs0 =
      req0->getInputs();  // const std::vector<InferenceRequestInput>

    try {
      migraphx::program_parameters params;

      // populate the migraphx parameters with shape read from the onnx
      // model.
      auto param_shapes = prog_.get_parameter_shapes();

      for (const auto& aninput : inputs0) {  // InferenceRequestInput
        auto aname = aninput.getName();

        // Look up the shape by name in the model, but if there's only 1 input
        // then the name in the request isn't required to match.
        if (inputs0.size() == 1) {
          aname = param_shapes.names().front();
        }
        migraphx::shape modelshape = param_shapes[aname.c_str()];

        if (toDataType(modelshape.type()) != aninput.getDatatype()) {
          smsg.str("");
          smsg << "Migraph worker model and input data types don't match:   "
               << toDataType(modelshape.type()) << " vs "
               << aninput.getDatatype();
          throw(invalid_argument(smsg.str()));
        }

        // check that lengths() and type match
        auto llen = modelshape.lengths();
        // clang-format off
        //    compare each dimension of shapes except the 0'th (batch size)
        //  TODO(bpickrel): the following check works inconsistently between different example client scripts.
        // It accepts inputs from the yolo script but rejects hello_migraphx.py inputs
        // const auto& av_shape = aninput.getShape();  // vector<int64>
        // for(size_t ii = 1; ii < av_shape.size(); ii++)
        // {
        //   if( av_shape.size() != llen.size() || av_shape[ii] != llen[ii])
        //   {
        //     smsg.str("");
        //     smsg << "Migraph worker model and input shapes don't match for input \"" << aname << "\":   ";
        //     for(auto j : llen) smsg << j << ", ";
        //     smsg << " vs " ;
        //     for(auto j : av_shape) smsg << j << ", ";
        //     AMDINFER_LOG_DEBUG(logger, smsg.str());
        //     throw invalid_argument(smsg.str());
        //   }
        // }
        // clang-format on

        auto* a_data = aninput.getData();  //  void *
        params.add(aname.c_str(), migraphx::argument(modelshape, a_data));
      }
      // If there were fewer requests in the batch than the stated batch size,
      // pad the various input tensors with copies of the 0'th request's data.

      if (pad_batch_) {
        // for each named input channel
        for (const auto& aninput : inputs0) {
          auto aname = aninput.getName();
          // Look up the shape by name in the model, but if there's only 1 input
          // then the name in the request isn't required to match.
          if (inputs0.size() == 1) {
            aname = param_shapes.names().front();
          }
          auto* a_data = static_cast<char*>(aninput.getData());
          // For each empty slot in buffer, i.e. from end of real requests up to
          // batch size
          for (size_t req_idx = batch->getRequests().size();
               req_idx < batch_size_; req_idx++) {
            memcpy(a_data + req_idx * input_sizes_[aname], a_data,
                   input_sizes_[aname]);
          }
        }
      }

      //
      // Run the inference
      //

      AMDINFER_LOG_INFO(logger, "Beginning migraphx eval");
      timer.add("eval_start");
      migraphx::api::arguments migraphx_output = this->prog_.eval(params);
      timer.add("eval_end");
      auto eval_duration_us = timer.count<std::micro>("eval_start", "eval_end");
      auto eval_duration_s = eval_duration_us / std::mega::num;
      AMDINFER_LOG_INFO(
        logger, std::string("Finished migraphx eval; batch size: ") +
                  std::to_string(batch_size_) + "  elapsed time: " +
                  std::to_string(eval_duration_us) + " us.  Images/sec: " +
                  std::to_string(batch_size_ / (eval_duration_s)));

      //
      //           Fetch the results and populate response to each request in
      //           the batch
      //

      // for each request in the batch
      for (unsigned int j = 0; j < batch->size(); j++) {
        const auto& req = batch->getRequest(j);
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

          // Fetch the vector shape, data, etc. for output from the
          // parsed/compiled model
          migraphx::api::shapes output_shapes = prog_.get_output_shapes();

          //
          // Transfer the migraphx results to output
          //
          size_t result_size =
            migraphx_output.size();  //   Resnet models have 1 output; yolo and
                                     //   bert models have 3

          // For each output channel in result:
          //
          for (size_t i = 0; i < result_size; i++) {
            // the buffer to populate for return
            InferenceResponseOutput output;

            migraphx_shape_datatype_t output_type = output_shapes[i].type();
            amdinfer::DataType output_dt = toDataType(output_type);
            output.setDatatype(output_dt);

            auto this_output = migraphx_output[i];
            migraphx::api::shape shape = this_output.get_shape();
            auto lengths = shape.lengths();

            size_t num_results = std::accumulate(
              lengths.begin() + 1, lengths.end(), 1, std::multiplies<>());

            // remove the 0'th dimension (batch size) from lengths
            lengths.erase(lengths.begin());
            // size of each result array, bytes
            size_t size_of_result = num_results * output_dt.size();

            // pointer to offset in data blob
            char* results = this_output.data() + j * size_of_result;

            // the kserve specification for response output is at
            // https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md#response-output
            //
            // The outputs buffer in the InferenceRequest is not used or
            // enforced at the time of writing this, but here it is. Give the
            // output a default name if necessary.
            auto outputs =
              req->getOutputs();  // one result vector for each request

            std::string output_name;
            if (i < outputs.size()) {
              output_name = outputs[i].getName();
            }

            if (output_name.empty()) {
              output.setName(inputs0[0].getName());
            } else {
              output.setName(output_name);
            }
            output.setShape(lengths);
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
#ifdef AMDINFER_ENABLE_METRICS
          Metrics::getInstance().incrementCounter(
            MetricCounterIDs::PipelineEgressWorker);
          timer.add("batch_time", batch->getTime(j));
          timer.add("request_latency");
          auto duration =
            timer.count<std::micro>("batch_time", "request_latency");
          Metrics::getInstance().observeSummary(
            MetricSummaryIDs::RequestLatency, duration);
#endif
        } catch (const std::exception& e) {
          AMDINFER_LOG_ERROR(logger, e.what());
          // Pass error message back as reply to request; continue processing
          // more inference requests

          req->runCallbackError(
            std::string("Error processing Migraphx request: ") + e.what());
        }
      }  // end j, request
    } catch (const std::exception& e) {
      // This outer catch block catches exceptions in evaluation of the batch.
      AMDINFER_LOG_ERROR(logger, e.what());
      // Pass error message back as reply for each request in the batch
      const auto& requests = batch->getRequests();
      for (const auto& req_e : requests) {
        req_e->runCallbackError(std::string("Migraphx inference error: ") +
                                e.what());
      }
    }

    timer.add("batch_stop");
    auto duration = timer.count<std::micro>("batch_start", "batch_stop");
    AMDINFER_LOG_INFO(
      logger, std::string("Finished migraphx batch processing; batch size: ") +
                std::to_string(batch_size_) +
                "  elapsed time: " + std::to_string(duration) + " us");
  }  // end while (batch)
  AMDINFER_LOG_INFO(logger, "Migraphx::doRun ending");
}

void MIGraphXWorker::doRelease() {}
void MIGraphXWorker::doDeallocate() {}
void MIGraphXWorker::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::MIGraphXWorker("MIGraphX", "gpu");
}
}  // extern C
