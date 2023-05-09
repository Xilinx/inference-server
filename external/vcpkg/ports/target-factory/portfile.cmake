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
  REPO Xilinx/Vitis-AI
  REF "v3.0"
  SHA512 4656943dcce918c9dfe79602168ddf0ee40446a7574c6372410d0f37bf7df0fe4f16d0f955b30bae472d0c08c32b3ae608df697152bf07ae630d5e99a7663718
  HEAD_REF master
  PATCHES "remove-git-dependency.patch"
)

vcpkg_cmake_configure(
  SOURCE_PATH ${SOURCE_PATH}/src/vai_runtime/target_factory
  OPTIONS
    -DPROJECT_GIT_COMMIT_ID="2057c588c241c3902353bc618afc8ab214d7aeed"
    -DPROJECT_GIT_BRANCH_NAME="master"
)
# cmake-format: on

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH share/cmake/target-factory)

vcpkg_copy_pdbs()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")