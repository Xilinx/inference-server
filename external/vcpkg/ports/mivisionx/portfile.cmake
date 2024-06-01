# Copyright 2024 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# cmake-format: off
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO ROCm/MIVisionX
  REF d3496f049c82b362a51e7a1dd8eb71ec76bd2c1c
  SHA512 57ffbfe8eb12a9d48cd329d1819f3961d61a3e6ea47412e6e3a701b91896418d7137baf3486c010b4ec7c9f430ae28885c8e011c6b1cb250f57040688bd28f09
  HEAD_REF master
)

vcpkg_cmake_configure(
  SOURCE_PATH ${SOURCE_PATH}
  OPTIONS
    -DNEURAL_NET=OFF
    -DROCAL=OFF
    -DROCAL_PYTHON=OFF
    -DLOOM=OFF
    -DMIGRAPHX=OFF
    -DBACKEND=CPU
)
# cmake-format: on

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_copy_pdbs()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")