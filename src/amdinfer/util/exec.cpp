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

#include "amdinfer/util/exec.hpp"

#include <array>              // for array
#include <boost/process.hpp>  // for process, std_out_, null, std_e...
#include <cassert>            // for assert
#include <istream>            // for basic_istream<>::__istream_type
#include <string>             // for string
#include <vector>             // for vector

namespace bp = boost::process;

namespace amdinfer::util {

std::string exec(const char* cmd) {
  // this function call, and the bp::system call below raise a possible nullptr
  // warning somewhere in Boost headers. This NOLINT suppresses it
  // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
  assert(cmd != nullptr);
  const auto chunk_size = 128;

  bp::ipstream is;
  // stdout and stderr cannot be redirected to the same stream
  // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
  bp::system(cmd, bp::std_out > is, bp::std_err > bp::null);

  std::string output;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
  std::array<char, chunk_size> out_buffer;
  output.reserve(chunk_size);

  while (is.read(out_buffer.data(), out_buffer.size())) {
    // if read is true, then a whole chunk has been read, copy over all of it
    output.append(out_buffer.data(), out_buffer.size());
  }
  // copy over whatever is left
  output.append(out_buffer.data(), is.gcount());
  return output;
}

}  // namespace amdinfer::util
