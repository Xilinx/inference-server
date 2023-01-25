..
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

GPUs - MIGraphX
===============

Using the AMD Inference Server with MIGraphX and GPUs requires some additional setup prior to use.

Set up the host and GPUs
------------------------

Prior to installing the Inference Server, first ensure your system recognizes your GPU(s).
Start by following the `ROCm installation instructions <https://docs.amd.com/category/ROCm%E2%84%A2%20v5.x>`__ for version 5.4.1 or newer.
Once your system recognizes your GPU(s), proceed to the next step.

Build an image
--------------

To build an image with MIGraphX enabled, you need to add the ``--migraphx`` to the ``amdinfer dockerize`` command:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the dev image $(whoami)/amdinfer-dev-migraphx:latest
    ./amdinfer dockerize --migraphx --suffix="-migraphx"

    # build the deployment image $(whoami)/amdinfer-migraphx:latest
    ./amdinfer dockerize --migraphx --suffix="-migraphx" --production

Start an image
--------------

The development container can be started with:

.. code-block:: console

    $ ./amdinfer run --dev

This automatically adds the detected devices, publishes ports, and mounts some convenient directories, such as your SSH directory, and drops you into a terminal in the container.

You can start the :ref:`deployment container on Docker <docker:start the container>` with something like:

.. code-block:: console

    $ docker run --device /dev/kfd --device /dev/dri [--volume ...]

These ``--device`` flags pass the GPU to the container and you can mount other directories as needed to make models available.

Get assets and models
---------------------

You can download the assets and models used for tests and examples with:

.. code-block:: console

    $ ./amdinfer get --migraphx
