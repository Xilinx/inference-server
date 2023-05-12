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

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# set(VCPKG_C_FLAGS ${VCPKG_C_FLAGS} -std=c++17)
# set(VCPKG_CXX_FLAGS ${VCPKG_CXX_FLAGS} -std=c++17)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)
# adds $ORIGIN to the RUNPATH so shared libraries are relocatable
set(VCPKG_FIXUP_ELF_RPATH TRUE)
