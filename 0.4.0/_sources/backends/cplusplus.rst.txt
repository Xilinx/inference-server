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

CPlusPlus
=========

The CPlusPlus backend can run arbitrary C++ code targeting any hardware accelerator that adheres to the expected interface.

Model support
-------------

Since this backend is for running C++ code, anything you can compile to a shared library correctly should work with this backend.
It supports models with multiple input and output tensors.

Hardware support
----------------

Depending on the content of the shared library, the backend could target CPUs, GPUs or FPGAs or other hardware accelerators.
You will need any compile-time or run-time dependencies of what you are running if it requires additional software.

Host setup
----------

Any setup needed for the host will depend on which models you want to deploy and what hardware/software they use.
Follow the directions for the original tools you are using.

Build an image
--------------

To build an image with the CPlusPlus backend enabled, you don't need to add any special flags.
It is always enabled.

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the development image $(whoami)/amdinfer-dev:latest
    ./amdinfer dockerize

    # build the development image $(whoami)/amdinfer-dev-basic:latest
    ./amdinfer dockerize --suffix="-basic"

    # build the deployment image $(whoami)/amdinfer-basic:latest
    ./amdinfer dockerize --suffix="-basic" --production

Start a container
-----------------

Depending on your use case and how you are using the server, you can start a container to use this backend in multiple ways.

Deployment
^^^^^^^^^^

You can start a deployment container with something like:

.. code-block:: console

    $ docker run [--device ...] [--volume ...]

Depending on the requirements of the models you want to deploy, you may need to pass hardware devices to the container using the ``--device`` flags or mount volumes with ``--volume`` flags.

Development
^^^^^^^^^^^

A development container can be started with:

.. code-block:: console

    $ ./amdinfer run --dev

This automatically publishes ports and mounts some convenient directories, such as your SSH directory, and drops you into a terminal in the container.

Get test assets
---------------

Assets and models used with this backend are always downloaded so you can get them with any flags to:

.. code-block:: console

    $ ./amdinfer get

Loading the backend
-------------------

.. include:: /dry.rst
    :start-after: +loading_the_backend_intro
    :end-before: -loading_the_backend_intro

.. tabs::

    .. code-tab:: c++ C++

        // amdinfer::Client* client;
        // amdinfer::ParameterMap parameters;
        std::string endpoint = client->workerLoad("cplusplus", parameters)

    .. code-tab:: python Python

        # client = amdinfer.Client()
        # parameters = amdinfer.ParameterMap()
        endpoint = client.workerLoad("cplusplus", parameters)

.. include:: /dry.rst
    :start-after: +loading_the_backend_modelLoad
    :end-before: -loading_the_backend_modelLoad

Parameters
^^^^^^^^^^

You can provide the following backend-specific parameters at load-time:

.. csv-table::
    :header: Parameter,Type,Usage

    ``batch_size``,integer,Requested batch size for incoming batches
    ``model``,string,Full path to the shared library to load. Alternatively, you can pass a string not ending with ``.so`` and the backend will convert it to ``lib<model>.so`` and use the runtime loader to load this shared library.

Troubleshooting
---------------

If you run into problems, first check the :ref:`general troubleshooting guide <troubleshooting:Troubleshooting>` guide.
Then continue on to this CPlusPlus specific troubleshooting guide.
You will need access to the machine where the inference server is running to debug.

TODO

.. |platform| replace:: ``amdinfer_cpp``
