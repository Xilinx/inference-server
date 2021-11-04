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

#include "proteus/core/interface.hpp"

#include <utility>  // for move

#include "proteus/build_options.hpp"        // for PROTEUS_ENABLE_TRACING
#include "proteus/observation/logging.hpp"  // for getLogger, LoggerPtr
#include "proteus/observation/tracing.hpp"  // for SpanPtr

namespace proteus {

Interface::Interface() {
#ifdef PROTEUS_ENABLE_LOGGING
  this->logger_ = getLogger();
#endif
  this->type_ = InterfaceType::kUnknown;
#ifdef PROTEUS_ENABLE_TRACING
  this->span_ = nullptr;
#endif
}

InterfaceType Interface::getType() { return this->type_; }

#ifdef PROTEUS_ENABLE_TRACING
void Interface::setSpan(SpanPtr span) { this->span_ = std::move(span); }
opentracing::Span* Interface::getSpan() { return this->span_.get(); }
#endif

}  // namespace proteus
