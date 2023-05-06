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
  REPO SpartanJ/efsw
  REF "6b51944994b5c77dbd7edce66846e378a3bf4d8e"
  SHA512 f49a8e5f4ec2d05c3d7bcdbfc6b39a9f1bb9667881cb52c065d038b558af0bfe19079bc9937398fc15faab680c1b2a685d00321143a5c324c8a31137549a7d0f
  HEAD_REF master
)

vcpkg_cmake_configure(
  SOURCE_PATH ${SOURCE_PATH}
  OPTIONS -DVERBOSE=OFF -DBUILD_TEST_APP=OFF -DLEGACY=OFF
)
# cmake-format: on

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/efsw)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_copy_pdbs()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
