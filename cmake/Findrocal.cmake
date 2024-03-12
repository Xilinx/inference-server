# # Copyright 2024 Advanced Micro Devices, Inc.
# #
# # Licensed under the Apache License, Version 2.0 (the "License");
# # you may not use this file except in compliance with the License.
# # You may obtain a copy of the License at
# #
# #      http://www.apache.org/licenses/LICENSE-2.0
# #
# # Unless required by applicable law or agreed to in writing, software
# # distributed under the License is distributed on an "AS IS" BASIS,
# # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# # See the License for the specific language governing permissions and
# # limitations under the License.

# # Look for an executable called tensorflow_cc
# find_path(ROCAL_INCLUDE_DIR 
#     NAMES rocal_api.h
#     PATHS /opt/vcpkg/x64-linux-dynamic/include/rocal/ 
# )

# find_library(ROCAL_LIBRARIES
#     NAMES rocal
#     PATHS /opt/vcpkg/x64-linux-dynamic/lib/
# )

# if(ROCAL_INCLUDE_DIR AND ROCAL_LIBRARIES)
#     set(rocal_FOUND)
# endif()

# include(FindPackageHandleStandardArgs)

# # Handle standard arguments to find_package like REQUIRED and QUIET
# find_package_handle_standard_args(
#   rocal "Failed to find rocal library"
# )
