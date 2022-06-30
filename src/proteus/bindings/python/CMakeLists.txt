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
  set(CXX_STANDARD_FLAG
      ${CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION}
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/auto_docstrings.hpp
    COMMAND
      pybind11-mkdoc -o ${CMAKE_CURRENT_BINARY_DIR}/auto_docstrings.hpp
      ${ALL_PUBLIC_HEADER_FILES} ${CXX_STANDARD_FLAG}
      -I${PROTEUS_PUBLIC_INCLUDE_DIRS}
    DEPENDS ${ALL_PUBLIC_HEADER_FILES}
  )
else()
  configure_file(
    helpers/auto_docstrings.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/auto_docstrings.hpp
  )
endif()

add_custom_target(
  pybind11Mkdoc DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/auto_docstrings.hpp
)

add_subdirectory(core) # sets CORE_OBJS
add_subdirectory(clients) # sets CLIENT_OBJS

# add this dummy file to add a dependency on the Pybind11 target so it runs if
# the Proteus package isn't installed in pip
set(dummy_new_file ${CMAKE_CURRENT_BINARY_DIR}/src/main.cpp)
if(NOT EXISTS ${dummy_new_file})
  file(WRITE ${dummy_new_file} "")
endif()
execute_process(
  COMMAND sh -c "pip list | grep Proteus > /dev/null" RESULT_VARIABLE Result
)
if(${Result})
  # touch the file to modify it if pip fails to find the package
  file(TOUCH ${dummy_new_file})
endif()

pybind11_add_module(proteus_py MODULE proteus.cpp ${dummy_new_file})
target_include_directories(proteus_py PRIVATE ${PROTEUS_INCLUDE_DIRS})
target_link_libraries(
  proteus_py PRIVATE proteus ${Python_LIBRARIES} ${CORE_OBJS} ${CLIENT_OBJS}
)
set_target_properties(proteus_py PROPERTIES OUTPUT_NAME _proteus)

find_package(Pybind11Stubgen)
if(${Pybind11Stubgen_FOUND})
  add_custom_command(
    TARGET proteus_py POST_BUILD
    COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} pybind11-stubgen -o
            ${CMAKE_CURRENT_BINARY_DIR} _proteus --no-setup-py
  )
else()
  message(
    STATUS "Pybind11Stubgen executable not found, not generating Python stubs"
  )
endif()

# copy over Python package source files to the build tree
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/setup.py ${CMAKE_CURRENT_BINARY_DIR}/setup.py
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/__init__.py
  ${CMAKE_CURRENT_BINARY_DIR}/src/proteus/__init__.py COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/clients/__init__.py
  ${CMAKE_CURRENT_BINARY_DIR}/src/proteus/clients/__init__.py COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/predict_api/__init__.py
  ${CMAKE_CURRENT_BINARY_DIR}/src/proteus/predict_api/__init__.py COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/py.typed
  ${CMAKE_CURRENT_BINARY_DIR}/src/proteus/py.typed COPYONLY
)

execute_process(
  COMMAND ${Python_EXECUTABLE} -m site --user-site
  OUTPUT_VARIABLE PY_USER_SITE_PACKAGES
  OUTPUT_STRIP_TRAILING_WHITESPACE
  # COMMAND_ECHO STDOUT
  COMMAND_ERROR_IS_FATAL ANY
)

add_custom_command(
  TARGET proteus_py
  POST_BUILD
  COMMAND pip install --prefix ~/.local .
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/*.so ${PY_USER_SITE_PACKAGES}
  # COMMENT "Add ${CMAKE_CURRENT_BINARY_DIR} to the PYTHONPATH to use the
  # proteus Python package"
)

# we need to run this after the pip install above
if(${Pybind11Stubgen_FOUND})
  add_custom_command(
    TARGET proteus_py POST_BUILD
    COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} pybind11-stubgen -o
            ${CMAKE_CURRENT_BINARY_DIR} proteus --no-setup-py
  )
endif()
