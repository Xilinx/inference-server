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
  REF bbd45838d4a93f894cfc9f232140dc65af2398d1
  SHA512 fc7cae389593416de5812298aebbaf61e9df2684f6746bcebca5f172813e4b71bb2009edfb0235f4f00b7382a540e4bf1c517d1711195e2af2cd6a49945c50ac
  HEAD_REF master
  PATCHES "remove-git-dependency.patch"
)

vcpkg_cmake_configure(
  SOURCE_PATH ${SOURCE_PATH}/src/vai_runtime/target_factory
  OPTIONS
    -DPROJECT_GIT_COMMIT_ID="bbd45838d4a93f894cfc9f232140dc65af2398d1"
    -DPROJECT_GIT_BRANCH_NAME="master"
)
# cmake-format: on

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH share/cmake/target-factory)

vcpkg_copy_pdbs()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# remove duplicate include files from the debug/* directory per vcpkg warning
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
