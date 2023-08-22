# Copyright 2023 Advanced Micro Devices, Inc.
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
  REPO Xilinx/rt-engine
  REF "v3.5"
  SHA512 2827b0cbcf3bc56b7c6396ba54bfbb43eac73fc5b6ae65d3a0c77f84fc1dcbf7a1eff192555b250ff6f2bb90508c1b2bbb6feaad4f97842e3b52ddfea2b945af
  HEAD_REF master
  PATCHES fix-cmake.patch
)

vcpkg_cmake_configure(
  SOURCE_PATH ${SOURCE_PATH}
  OPTIONS
    "-DXRM_DIR=/opt/xilinx/xrm/share/cmake"
    "-DBUILD_TESTS=OFF"
)
# cmake-format: on

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH share/cmake/rt-engine)

vcpkg_copy_pdbs()

# there's no license file in rt-engine
# vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
