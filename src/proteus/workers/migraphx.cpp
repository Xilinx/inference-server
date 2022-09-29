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
#include "proteus/util/thread.hpp"            // for setThreadName
#include "proteus/workers/worker.hpp"         // for Worker

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
  // unsigned output_classes_;
  // unsigned image_width_, image_height_, image_channels_, image_size_;

  // If we're using a model with one input, store its shape here.  If more than
  // one, read their shapes from the model for eval.  (We don't do this
  // for the single input case only because they're looked up by name, and
  // single-input clients aren't guaranteed to give a correct name)
  // migraphx::shape input_shape;

  // Input / Output nodes
  std::string input_node_, output_node_;

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
// stringstream used for formatting logger messages
std::string msg;
std::stringstream smsg(msg);

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
      PROTEUS_LOG_INFO(logger,
                       std::string("migraphx worker ASDF loaded ONNX model file ") +
                         onnx_path.c_str());

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
  // Fetch the expected dimensions of the input from the parsed model. 
  migraphx::program_parameter_shapes input_shapes =
    this->prog_.get_parameter_shapes();
  auto input_name = input_shapes.names()[0];
  auto sh = input_shapes[input_name]; // migraphx::shape
  auto length = sh.lengths();
  migraphx::api::shapes output_shapes = prog_.get_output_shapes();
  this->batch_size_ = length[0];

  // std::vector<size_t> output_lengths = output_shapes[0].lengths();
  // output_classes_ = std::accumulate(
  //   output_lengths.begin(), output_lengths.end(), 1, std::multiplies<size_t>());
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
  PROTEUS_LOG_INFO(logger, "MIGraphXWorker::doAllocate");
  //
  // Allocate
  //
  constexpr auto kBufferNum = 3U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  // Allocate enough to hold buffer_num batches' worth of images.
  // Extra batches allow server to hold more requests at one time
  // todo:  this try/catch was observed to just get stuck when batch size
  // is too big (approx. 56 for Yolov4 model); how to catch the error?

  // Calculate the total number of bytes required for all inputs
  
  size_t in_buffer_size{0};
  (void)in_buffer_size;
  BufferPtrs buffer_vec;

  migraphx::program_parameter_shapes input_shapes = this->prog_.get_parameter_shapes();

  // Work out the max. size of any input buffer, in bytes.  We'll allocate all of them the same size.
  size_t max_buffer(0);
  for(auto aname :  input_shapes.names()){

    migraphx::shape ashape = input_shapes[aname];
    auto llen = ashape.lengths();
    size_t this_input_size = std::accumulate(
        llen.begin() + 1, llen.end(), 1, std::multiplies<size_t>());   // <== skip 0'th dimension, which is batch size
    this_input_size *= (toDataType(ashape.type()).size());
    max_buffer = std::max(max_buffer, this_input_size);
  }

  // Now, allocate the input and output buffers.
    
  try{  
    for(auto aname : input_shapes.names()){ 
      auto ashape = input_shapes[aname];
      auto llen = ashape.lengths();

      // todo: test whether VectorBuffer::allocate() does this in the right order for multiple (kBufferNum) sets of buffers.
      // It wasn't designed to be called in a loop like this.  Using 1 in place of kBufferNum
      // VectorBuffer::allocate(
      //   this->input_buffers_, 1,
      //   max_buffer,
      //   DataType::UINT8);
                             
                             
                             
      buffer_vec.emplace_back(std::make_unique<VectorBuffer>(max_buffer, DataType::UINT8));
  PROTEUS_LOG_INFO(logger, std::string("buffer_vec has size   ") +
                             std::to_string(buffer_vec.size()) + " buffers");
    }
    this->input_buffers_->enqueue(std::move(buffer_vec));

    // Calculate output buffer size 
    size_t out_buffer_size{0};
    migraphx::shapes output_shapes = this->prog_.get_output_shapes();
    for(auto ash : output_shapes){ 
      auto llen = ash.lengths();
      size_t this_output_size = std::accumulate(
          llen.begin(), llen.end(), 1, std::multiplies<size_t>());
          
      this_output_size *= (toDataType(ash.type()).size());
      out_buffer_size += this_output_size;
    }

    // Allocating all output buffers as one block seems to work fine
    VectorBuffer::allocate(this->output_buffers_, buffer_num,
                          out_buffer_size, proteus::DataType::INT8);
  } catch (...) {
      PROTEUS_LOG_ERROR(logger, std::string("MIGraphXWorker couldn't allocate buffer (batch size ") + std::to_string(batch_size_) + ")");
      throw "MIGraphXWorker couldn't allocate buffer";
  }
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
    PROTEUS_LOG_INFO(logger, "New batch request in migraphx");
    std::chrono::time_point batch_tp =
      std::chrono::high_resolution_clock::now();
#ifdef PROTEUS_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::kPipelineIngressWorker);
#endif

    // The MIGraphX operation: run the migraphx eval() method.
    // If migraphx exceptions happen, they will be handled

    // We only need to look at the 0'th request to set up evaluation, because
    // its input pointers (one for each input) are the base addresses of the data for the entire batch.
    // The different input tensors are not required to be contiguous with each other.
    auto& req0 = batch->getRequest(0);
    auto inputs0 = req0->getInputs();   // const std::vector<InferenceRequestInput>

    try {
      migraphx::program_parameters params;

      // populate the migraphx parameters with shape read from the onnx
      // model.
      auto param_shapes = prog_.get_parameter_shapes();

      for ( auto aninput : inputs0 ){ // InferenceRequestInput
        auto aname = aninput.getName();
        auto avShape = aninput.getShape();  // vector<int64>

        // check that lengths() and type match

        // Look up the shape by name in the model, but if there's only 1 input then
        // the name in the request isn't required to match.
        if(inputs0.size() == 1)
        {
          aname = param_shapes.names().front();
        }
        migraphx::shape modelshape = param_shapes[aname.c_str()];
        
        if( toDataType(modelshape.type()) != aninput.getDatatype()){
          smsg.str("");
smsg << "model and input data types don't match:   " << toDataType(modelshape.type()) << " vs " << aninput.getDatatype();
PROTEUS_LOG_DEBUG(logger, smsg.str());
        }

        auto llen = modelshape.lengths();
        // compare each dimension of shapes except the 0'th (batch size)
        for(size_t ii = 1; ii < avShape.size(); ii++)
        {  
          if( avShape.size() != llen.size() ||    avShape[ii] != llen[ii])
          {
          smsg.str("");
smsg << "model and input shapes don't match:   ";
for(auto j : llen) smsg << j << ", ";
smsg << " vs " ;
for(auto j : avShape) smsg << j << ", ";
PROTEUS_LOG_DEBUG(logger, smsg.str());
// throw(invalid_argument(smsg.str()));
          }
        }

PROTEUS_LOG_DEBUG(logger, (aname + "  is input name").c_str());


        auto aData = aninput.getData();  //  void *
        params.add(aname.c_str(), migraphx::argument(modelshape, aData));

      }

      // TODO: if there were fewer requests in the batch than the stated batch size,
      // pad the various input tensors with copies of the 0'th request's data.

      //
      // Run the inference
      //


      PROTEUS_LOG_INFO(logger, "Beginning migraphx eval");
      std::chrono::time_point eval_tp =
        std::chrono::high_resolution_clock::now();
      migraphx::api::arguments migraphx_output = this->prog_.eval(params);  // <==^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
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

          // Fetch the vector shape, data, etc. for output from the parsed/compiled model
          migraphx::api::shapes output_shapes = prog_.get_output_shapes();

          //
          // Transfer the migraphx results to output
          //
          size_t result_size =  migraphx_output.size();  //   Resnet models have 1 output; yolo and bert models have 3

          // For each output channel in result:
          //
          for(size_t i = 0; i < result_size; i++)
          {
            // the buffer to populate for return
            InferenceResponseOutput output;

            migraphx_shape_datatype_t output_type = output_shapes[i].type();
            proteus::DataType output_dt = toDataType(output_type);
            output.setDatatype(output_dt);

            auto this_output = migraphx_output[i];
            migraphx::api::shape shape = this_output.get_shape();
            auto lengths = shape.lengths();

            size_t num_results = std::accumulate(
              lengths.begin() + 1, lengths.end(), 1, std::multiplies<size_t>());

            // remove the 0'th dimension (batch size) from lengths
            lengths.erase(lengths.begin());
            // size of each result array, bytes
            size_t size_of_result = num_results * output_dt.size();

            // pointer to offset in data blob
            char* results =  this_output.data() + j * size_of_result;

            
            // the kserve specification for response output is at
            // https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md#response-output
            //
            // The outputs buffer in the InferenceRequest is not used or enforced at the time of writing this,
            // but here it is.
            // so give the output a default name if necessary.
            // The InferenceRequestOutput items 
            auto outputs = req->getOutputs();  // one result vector for each request

            std::string output_name{""};
            if(i < outputs.size())
              output_name = outputs[i].getName();
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

            PROTEUS_LOG_DEBUG(logger, (std::string("Adding an output")));
            smsg.str("dimensions: ");
            for(auto a: lengths)
              smsg << a << "  ";
            PROTEUS_LOG_DEBUG(logger, smsg.str());
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
