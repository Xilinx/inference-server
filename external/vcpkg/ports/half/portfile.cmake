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
vcpkg_from_sourceforge(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO half/half
  REF 2.2.0
  FILENAME "half-2.2.0.zip"
  SHA512 299db9ab12d8135ae2eb1bb6229e11fec72c1ccc4d0db04d1c912c1e5cdadbddec738097b03c3bbf9b7993e589c32abcd509bfe0c20daf1996da65f633a94574
  NO_REMOVE_ONE_LEVEL
)

file(
  INSTALL "${SOURCE_PATH}/include/half.hpp"
  DESTINATION "${CURRENT_PACKAGES_DIR}/include/half"
  FILES_MATCHING PATTERN "*.hpp"
)
# cmake-format: on

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
