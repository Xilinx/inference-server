..
    Copyright 2023 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

MLCommons
=========

Prerequisites
-------------

- CMake
- git
- amdinfer


Build the app
-------------

Build and install the MLCommons loadgen library:

.. code-block:: bash

    # cannot do shallow clone because loadgen build requires a git repository
    git clone --single-branch -b v2.0 https://github.com/mlcommons/inference.git mlcommons_inference
    cd mlcommons_inference/loadgen
    cmake -S . -B build
    cmake --build build -- -j4
    # the normal install target installs everything to CMAKE_INSTALL_PREFIX/* which
    # can lead to name collisions so manually create install directories
    export LIB_INSTALL_PATH=/usr/local/lib
    export INC_INSTALL_PATH=/usr/local/include/mlcommons/loadgen
    sudo mkdir -p $LIB_INSTALL_PATH
    sudo mkdir -p $INC_INSTALL_PATH
    sudo cp ./build/libmlperf_loadgen.a $LIB_INSTALL_PATH
    sudo cp *.h $INC_INSTALL_PATH

You can also refer to the `official build instructions <https://github.com/mlcommons/inference/blob/master/loadgen/README_BUILD.md>`__

Build the app:

.. code-block:: bash

    cmake -S . -B build
    cmake --build build -- -j4

Run the app
-----------

.. code-block:: bash

    ./build/src/app <flags>

Use ``--help`` to see the options.
