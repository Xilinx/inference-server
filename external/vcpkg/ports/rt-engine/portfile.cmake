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
  REF "v3.0"
  SHA512 9263ed06938fe7eae91cfc25fea0de28596b3f15b5eabe2c9095ee7046123916ca38b7c7c7962eaf58851b22c90d14f12965497bc7ded9ec623f3a584d3a2f00
  HEAD_REF master
  PATCHES fix-cmake.patch
  PATCHES fix-get_xclbins_in_dir.patch
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
