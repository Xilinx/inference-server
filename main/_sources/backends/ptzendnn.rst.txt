..
    Copyright 2022 Xilinx, Inc.
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

PtZenDNN
========

The PtZenDNN backend executes a PyTorch model on CPUs.
On AMD CPUs, the ZenDNN library offers optimized performance.

Model support
-------------

The PtZenDNN backend currently only supports ResNet50.

To use PyTorch models with this backend , you need to `convert downloaded PyTorch Eager models to TorchScript <https://pytorch.org/tutorials/advanced/cpp_export.html#step-1-converting-your-pytorch-model-to-torch-script>`_.

Hardware support
----------------

Check the `support matrix <https://www.amd.com/content/dam/amd/en/documents/developer/zendnn-support-matrix-4.0.pdf>`__ for compatible AMD CPUs.

Host setup
----------

No special host setup is required to use the PtZenDNN backend.

Build an image
--------------

To build an image with the PtZenDNN backend enabled, you need to download the ZenDNN library and then build the image by pointing the build script to the location of this downloaded package.

You can download the PT+ZenDNN package from the `ZenDNN developer downloads <LinkZenDNNdownload_>`_: ``PT_v1.12_ZenDNN_v4.0_C++_API.zip``.
Before downloading this packages, you will be required to read and agree to the :term:`EULA`.

After downloading the package, place it in the root of the repository.
To build an image with the backend enabled, you need to add the ``--ptzendnn`` flag to the ``amdinfer dockerize`` command and pass the file to the package:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the development image $(whoami)/amdinfer-dev:latest
    ./amdinfer dockerize --ptzendnn=./PT_v1.12_ZenDNN_v4.0_C++_API.zip

    # build the development image $(whoami)/amdinfer-dev-zendnn:latest
    ./amdinfer dockerize --ptzendnn=./PT_v1.12_ZenDNN_v4.0_C++_API.zip --suffix="-zendnn"

    # build the deployment image $(whoami)/amdinfer-zendnn:latest
    ./amdinfer dockerize --ptzendnn=./PT_v1.12_ZenDNN_v4.0_C++_API.zip --suffix="-zendnn" --production

.. note::

    The downloaded ZenDNN package will be used by the Docker build process so it must be in the inference server repository directory and in a location that is not excluded by the ``.dockerignore`` file.
    These instructions suggest using the repository root but any path that meets this criteria will work.

Start a container
-----------------

Depending on your use case and how you are using the server, you can start a container to use this backend in multiple ways.

Deployment
^^^^^^^^^^

You can start a deployment container with something like:

.. code-block:: console

    $ docker run ...

Development
^^^^^^^^^^^

A development container can be started with:

.. code-block:: console

    $ ./amdinfer run --dev

This automatically publishes ports and mounts some convenient directories, such as your SSH directory, and drops you into a terminal in the container.

Get test assets
---------------

You can download the assets and models used with this backend for tests and examples with:

.. code-block:: console

    $ ./amdinfer get --ptzendnn --all-models

Loading the backend
-------------------

.. include:: /dry.rst
    :start-after: +loading_the_backend_intro
    :end-before: -loading_the_backend_intro

.. tabs::

    .. code-tab:: c++ C++

        // amdinfer::Client* client;
        // amdinfer::ParameterMap parameters;
        std::string endpoint = client->workerLoad("ptzendnn", parameters)

    .. code-tab:: python Python

        # client = amdinfer.Client()
        # parameters = amdinfer.ParameterMap()
        endpoint = client.workerLoad("ptzendnn", parameters)

.. include:: /dry.rst
    :start-after: +loading_the_backend_modelLoad
    :end-before: -loading_the_backend_modelLoad

Parameters
^^^^^^^^^^

You can provide the following backend-specific parameters at load-time:

.. csv-table::
    :header: Parameter,Type,Usage

    ``batch_size``,integer,Requested batch size for incoming batches. Defaults to 1.
    ``image_channels``,integer,Number of channels in the input image. Defaults to 3.
    ``input_size``,integer,Assuming a square input image, the size of the image in pixels. Defaults to 224.
    ``model``,string,Full path to the model to load
    ``output_classes``,integer,Number of output classes in the classification model. Defaults to 1000.
    ``threads``,integer,Number of threads to use in the thread pool for the backend. Defaults to 3.

Troubleshooting
---------------

If you run into problems, first check the :ref:`general troubleshooting guide <troubleshooting:Troubleshooting>` guide.
Then continue on to this XModel specific troubleshooting guide.
You will need access to the machine where the inference server is running to debug.

Tune performance
^^^^^^^^^^^^^^^^

For tuning ZenDNN performance, you can refer to the PyTorch + ZenDNN `user guide <LinkZenDNNptGuide_>`_.

.. |platform| replace:: ``pytorch_torchscript``
