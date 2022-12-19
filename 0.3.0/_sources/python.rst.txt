..
    Copyright 2021 Xilinx, Inc.
    Copyright 2022 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Python
======

The Python library for the AMD Inference Server allows you to communicate with the server using Python.

Install the Python library
--------------------------

The Python library is built and installed in the development container as part of the regular CMake build.
To install it outside Docker or in different containers, you need to install a precompiled wheel.
Currently, these wheels are not available on Pip and must be built locally.

Build wheels
^^^^^^^^^^^^

You can build wheels for the Python library to make a precompiled package that can be installed in any Linux host, container or environment.
It is recommended to perform the following steps on a fresh clone of the inference server repository.
These instructions assume you're only building wheels for x86_64 Linux with CPython.

.. code-block:: bash

    # generate a Dockerfile that defines an image for building wheels
    ./docker/generate.py --cibuildwheel --base-image=quay.io/pypa/manylinux2014_x86_64 --base-image-type yum

    # build the image. You should add some suffix to differentiate this image
    # from the regular image
    ./amdinfer dockerize --suffix="-ci"

    # this will build an image with the name $(whoami)/amdinfer-dev-ci:latest
    # if you're not building wheels on the same host, you will need to upload
    # this image to a Docker registry

    # on a host where your image exists or can be pulled
    export CIBW_MANYLINUX_X86_64_IMAGE=$(whoami)/amdinfer-dev-ci:latest

    pip install cibuildwheel

    # you can edit pyproject.toml to control which wheels to build or just use the defaults

    cibuildwheel --platform linux

    # your built wheels will be in ./wheelhouse

After following these instructions, your built wheels will be in ``./wheelhouse/``.
The names on the wheels indicate the Python version they are compatible with.
For example, ``cp37`` in the name indicates that it's compatible with CPython 3.7.
You can install these wheels in a virtual environment, Conda environment, a container or on a bare host.

.. code-block:: console

    pip install <path/to/wheel>


API
---

.. automodule:: amdinfer
    :imported-members:
