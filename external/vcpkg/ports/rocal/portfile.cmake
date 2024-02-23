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
  REPO ROCm/rocAL
  REF 076b153daed5cf490292fc3ef59bc1ae4b237a42
  SHA512 30bea82d2db407e9e6fb0bda96b96380d714e7bb0ca4d3e6e7dbe1e7c791c1ab482ffda54a97de4e75295844596457e460f7633985ab342147a80ee616f71825
  HEAD_REF develop
)

# vcpkg_cmake_configure(
#   SOURCE_PATH ${SOURCE_PATH}
#   OPTIONS
#     -DBACKEND=CPU
#     -DBUILD_PYPACKAGE=OFF
# )
# cmake-format: on
# vcpkg_cmake_build()

# vcpkg_cmake_build(
#   TARGET PyPackageInstall
# )

# vcpkg_cmake_install()

# vcpkg_cmake_config_fixup(CONFIG_PATH share/cmake/rocAL)

# file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# vcpkg_copy_pdbs()

# vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
