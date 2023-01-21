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

#include "amdinfer/util/compression.hpp"

#include <zlib.h>  // for z_stream, inflate, inflateEnd, Z_OK, Z_NO_FLUSH

#include <array>    // for array
#include <cstring>  // for memset

namespace amdinfer::util {

std::string zDecompress(const char *str, int len) {
  const auto chunk_size = 32'768;

  // z_stream is zlib's control structure. Initialize to zero/null
  z_stream zs;
  memset(&zs, 0, sizeof(zs));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  zs.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(str));
  zs.avail_in = len;
  inflateInit(&zs);

  int ret = Z_OK;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
  std::array<char, chunk_size> out_buffer;
  std::string out_string;
  do {
    zs.next_out = reinterpret_cast<Bytef *>(out_buffer.data());
    zs.avail_out = out_buffer.size();
    ret = inflate(&zs, Z_NO_FLUSH);
    if (out_string.size() < zs.total_out) {
      out_string.append(out_buffer.data(), zs.total_out - out_string.size());
    }
  } while (ret == Z_OK);
  inflateEnd(&zs);
  if (ret == Z_STREAM_END) {
    return out_string;
  }
  return "";
}

}  // namespace amdinfer::util
