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

MIGraphX
========

The MIGraphX backend executes an ONNX or MXR model on an AMD GPU.

Model support
-------------

The MIGraphX backend should support most ONNX files and it supports models with multiple input and output tensors.
The tested models are listed below:

.. csv-table::
    :header: Model type,Tested models

    Classification,ResNet50
    Object detection,YOLOv4

Other models should also work but are currently untested.

MXR is an MIGraphX-compiled format.
When given an ONNX file, the MIGraphX backend will compile it to an MXR file.
This MXR file will be different depending on the GPU as well as some load-time parameters such as batch size.
If you move MXR files from one server to another, make sure they were compiled for the hardware on the new server.

Hardware support
----------------

While not every model is tested on every GPU, the MIGraphX backend has run at least one model on the following devices:

.. csv-table::
    :header: Classification,Name,ID

    CDNA,MI100,gfx908

Other devices and DPUs may also work but are currently untested.

Host setup
----------

Follow the `ROCm installation instructions <https://docs.amd.com/category/Release%20Documentation>`__ to install ROCm on your host machine.
Your ROCm version should ideally match the version installed in the inference server container but mismatching versions may work as well in some cases.
Ensure you can detect the GPUs on your host machine with ``/opt/rocm/bin/rocminfo``.

Build an image
--------------

To build an image with MIGraphX enabled, you need to add the ``--migraphx`` to the ``amdinfer dockerize`` command:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the development image $(whoami)/amdinfer-dev:latest
    ./amdinfer dockerize --migraphx

    # build the development image $(whoami)/amdinfer-dev-migraphx:latest
    ./amdinfer dockerize --migraphx --suffix="-migraphx"

    # build the deployment image $(whoami)/amdinfer-migraphx:latest
    ./amdinfer dockerize --migraphx --suffix="-migraphx" --production

Start a container
-----------------

Depending on your use case and how you are using the server, you can start a container to use this backend in multiple ways.

Deployment
^^^^^^^^^^

You can start a deployment container with something like:

.. code-block:: console

    $ docker run --device /dev/kfd --device /dev/dri [--volume ...]

These ``--device`` flags pass the GPU to the container and you can mount other directories as needed to make models available.

A development container can be started with:

.. code-block:: console

    $ ./amdinfer run --dev

This automatically adds the detected devices, publishes ports, and mounts some convenient directories, such as your SSH directory, and drops you into a terminal in the container.

Get test assets
---------------

You can download the assets and models used with this backend for tests and examples with:

.. code-block:: console

    $ ./amdinfer get --migraphx --all-models

Loading the backend
-------------------

.. include:: /dry.rst
    :start-after: +loading_the_backend_intro
    :end-before: -loading_the_backend_intro

.. tabs::

    .. code-tab:: c++ C++

        // amdinfer::Client* client;
        // amdinfer::ParameterMap parameters;
        std::string endpoint = client->workerLoad("migraphx", parameters)

    .. code-tab:: python Python

        # client = amdinfer.Client()
        # parameters = amdinfer.ParameterMap()
        endpoint = client.workerLoad("migraphx", parameters)

.. include:: /dry.rst
    :start-after: +loading_the_backend_modelLoad
    :end-before: -loading_the_backend_modelLoad

Parameters
^^^^^^^^^^

You can provide the following backend-specific parameters at load-time:

.. csv-table::
    :header: Parameter,Type,Usage

    ``batch``,integer,Requested batch size for incoming batches. Defaults to 64.
    ``model``,string,Full path to the model file to load
    ``pad_batch``,boolean,Use the first request to pad out the incoming batch if it contains fewer requests than the batch size. Defaults to true.

Troubleshooting
---------------

If you run into problems, first check the :ref:`general troubleshooting guide <troubleshooting:Troubleshooting>` guide.
Then continue on to this XModel specific troubleshooting guide.
You will need access to the machine where the inference server is running to debug.

Gotchas
^^^^^^^

Some common easy-to-make errors are listed here:

* MXR file mismatch: MXR files are compiled by the backend using the given ONNX file for the GPU and load-time parameters. The backend will also prefer to use MXR files if they exist. If you move the MXR files to a different server or use a network storage where the MXR files get used by different servers with different GPUs, the MXR file will not work and you will get cryptic errors. The solution is to delete the MXR file and let the backend recompile from the ONNX file for your hardware.

.. |platform| replace:: ``onnx_onnxv1`` or ``migraphx_mxr``
