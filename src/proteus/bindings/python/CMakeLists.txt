# Copyright 2022 Xilinx Inc.
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

find_package(Pybind11Mkdoc)
if(${Pybind11Mkdoc_FOUND})
    file(GLOB_RECURSE ALL_PUBLIC_HEADER_FILES CONFIGURE_DEPENDS
        ${PROTEUS_PUBLIC_INCLUDE_DIRS}/*.hpp
    )

    # get the "-std=c++xx" flag to pass to pybind11-mkdoc
    set(CXX_STANDARD_FLAG ${CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION})

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/docstrings.hpp
        COMMAND pybind11-mkdoc -o ${CMAKE_CURRENT_BINARY_DIR}/docstrings.hpp ${ALL_PUBLIC_HEADER_FILES} ${CXX_STANDARD_FLAG}
        DEPENDS ${ALL_PUBLIC_HEADER_FILES}
    )
else()
    configure_file(docstrings.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/docstrings.hpp)
endif()

add_custom_target(
    pybind11Mkdoc
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/docstrings.hpp
)

add_subdirectory(core) # sets CORE_OBJS
add_subdirectory(clients) # sets CLIENT_OBJS

pybind11_add_module(proteus_py MODULE proteus.cpp)
target_include_directories(proteus_py PRIVATE ${PROTEUS_INCLUDE_DIRS})
target_link_libraries(proteus_py PRIVATE proteus ${Python_LIBRARIES} ${CORE_OBJS} ${CLIENT_OBJS})
set_target_properties(proteus_py PROPERTIES OUTPUT_NAME proteus)

add_custom_command(TARGET proteus_py POST_BUILD
    COMMENT "Add ${CMAKE_CURRENT_BINARY_DIR} to the PYTHONPATH to use the proteus Python package"
)

find_package(Pybind11Stubgen)
if(${Pybind11Stubgen_FOUND})
    add_custom_command(TARGET proteus_py POST_BUILD
        COMMAND pybind11-stubgen -o ${CMAKE_CURRENT_BINARY_DIR} proteus --no-setup-py
    )
else()
    message(STATUS "Pybind11Stubgen executable not found, not generating Python stubs")
endif()
