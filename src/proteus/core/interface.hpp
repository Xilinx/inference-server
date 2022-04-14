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
 * @brief Defines the base Interface class
 */

#ifndef GUARD_PROTEUS_CORE_INTERFACE
#define GUARD_PROTEUS_CORE_INTERFACE

#include <chrono>     // for high_resolution_clock
#include <cstddef>    // for size_t
#include <memory>     // for shared_ptr, unique_ptr
#include <stdexcept>  // for invalid_argument
#include <vector>     // for vector

#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_TRACING
#include "proteus/helpers/declarations.hpp"  // for BufferRawPtrs
#include "proteus/observation/logging.hpp"   // for LoggerPtr
#include "proteus/observation/tracing.hpp"   // for SpanPtr, Span

namespace proteus {
class InferenceRequest;
}  // namespace proteus

namespace proteus {

/**
 * @brief The InterfaceType identifies the Interface. New interfaces can be
 * added by extending this enumeration.
 *
 */
enum class InterfaceType {
  kUnknown,
  kDrogonHttp,
  kDrogonWs,
  kCpp,
  kGrpc,
};

/**
 * @brief The Interface class represents an input-agnostic wrapper class that
 * can encapsulate incoming requests from different protocols and present a
 * consistent object and interface to the batcher to process all the requests.
 *
 */
class Interface {
 public:
  Interface();                     ///< Constructor
  virtual ~Interface() = default;  ///< Destructor
  /// Get the type of the interface
  InterfaceType getType();
#ifdef PROTEUS_ENABLE_TRACING
  /// Store the active trace into the Interface for propagation
  void setTrace(TracePtr &&trace);
  /// Get the stored trace from the interface for propagation
  TracePtr &&getTrace();
#endif
#ifdef PROTEUS_ENABLE_METRICS
  /// Store a time. This is used to track how long requests take
  void set_time(
    const std::chrono::high_resolution_clock::time_point &start_time);
  /// Get the stored time
  std::chrono::high_resolution_clock::time_point get_time();
#endif
  /// Get the number of input tensors in the request
  virtual size_t getInputSize() = 0;
  /**
   * @brief Construct an InferenceRequest using the data in the Interface
   *
   * @param buffer_index the index to use for the vector of I/O buffers
   * @param input_buffers a vector of buffers to hold the input data
   * @param input_offsets offsets of where to start storing input data
   * @param output_buffers a vector of buffers to hold the output data
   * @param output_offsets offsets of where to start storing output data
   * @param batch_size the requested batch size
   * @param batch_offset the current batch size
   * @return std::shared_ptr<InferenceRequest>
   */
  virtual std::shared_ptr<InferenceRequest> getRequest(
    size_t &buffer_index, const std::vector<BufferRawPtrs> &input_buffers,
    std::vector<size_t> &input_offsets,
    const std::vector<BufferRawPtrs> &output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset) = 0;

  /**
   * @brief Given an exception, handle the exception appropriately.
   *
   * @param e the raised exception
   */
  virtual void errorHandler(const std::invalid_argument &e) = 0;

 protected:
  InterfaceType type_;
#ifdef PROTEUS_ENABLE_LOGGING
  LoggerPtr logger_;
#endif
#ifdef PROTEUS_ENABLE_TRACING
  TracePtr trace_;
#endif
#ifdef PROTEUS_ENABLE_METRICS
  std::chrono::_V2::system_clock::time_point start_time_;
#endif
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_INTERFACE
