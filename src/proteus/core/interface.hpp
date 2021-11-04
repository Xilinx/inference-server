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

#ifndef GUARD_PROTEUS_CORE_INTERFACE
#define GUARD_PROTEUS_CORE_INTERFACE

#include <cstddef>    // for size_t
#include <memory>     // for shared_ptr, unique_ptr
#include <stdexcept>  // for invalid_argument
#include <vector>     // for vector

#include "proteus/build_options.hpp"        // for PROTEUS_ENABLE_TRACING
#include "proteus/observation/logging.hpp"  // for LoggerPtr
#include "proteus/observation/tracing.hpp"  // for SpanPtr, Span

namespace proteus {
class Buffer;
class InferenceRequest;
}  // namespace proteus

namespace proteus {

enum class InterfaceType {
  kUnknown,
  kDrogonHttp,
  kDrogonWs,
  kCpp,
};

class Interface {
 public:
  Interface();
  virtual ~Interface() = default;
  InterfaceType getType();
#ifdef PROTEUS_ENABLE_TRACING
  void setSpan(SpanPtr span);
  opentracing::Span *getSpan();
#endif
  virtual size_t getInputSize() = 0;
  virtual std::shared_ptr<InferenceRequest> getRequest(
    size_t &buffer_index, std::vector<BufferRawPtrs> input_buffers,
    std::vector<size_t> &input_offsets,
    std::vector<BufferRawPtrs> output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset) = 0;

  virtual void errorHandler(const std::invalid_argument &e) = 0;

 protected:
  InterfaceType type_;
#ifdef PROTEUS_ENABLE_LOGGING
  LoggerPtr logger_;
#endif
#ifdef PROTEUS_ENABLE_TRACING
  SpanPtr span_;
#endif
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_INTERFACE
