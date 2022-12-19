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

Quickstart - Inference
======================

The easiest way to use the AMD Inference Server is to run it inside a `Docker container <https://docs.docker.com/get-docker/>`__.
There are two modes of operation: development and deployment.
If you are just interested in deploying the server to serve inferences, then consider the :ref:`deployment instructions <Deployment>`.
To enable testing, debugging, and experimentation as well, follow the :ref:`development instructions <Development>`.

The inference server supports multiple platforms and hardware backends.
Ensure that you set up your host appropriately depending on which platform(s) you are using.
More information on host setup can be found in :ref:`each platforms' guide <platforms>`.

The helper script used for many of the commands here is :file:`amdinfer`: a Python script with many helpful options.
The most up-to-date documentation for this script can be seen with :ref:`online <cli:command-line interface>` or on the terminal with :option:`--help`.
You can also use :option:`--dry-run` before any command to see the underlying commands the script is running.

Deployment
----------

For deployment, the container for the AMD Inference server only contains the runtime dependencies of the server to minimize image size.
The deployment container, also referred to as the production container, contains a precompiled binary for the server that automatically starts when the container starts.

Build or get the Docker image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use ``docker pull`` to get the production container from a Docker registry if it's already built.

To build from source, you will need Docker 18.09 or newer because you need to enable `BuildKit <https://docs.docker.com/build/>`__ to build the image.
Look at the instructions for :ref:`building the production container <docker:Build the production Docker image>` for more information.

Launch the server
^^^^^^^^^^^^^^^^^

Your chosen deployment method determines how to start the server.

If you want to launch it as a bare Docker container, follow the :ref:`Docker instructions <docker:Prepare the image for Docker deployment>`.
This has the fewest external dependencies but requires more manual work to set up the image.

If you want to use the AMD Inference Server with `KServe <https://kserve.github.io/website/master/>`__, follow the :ref:`KServe instructions <kserve:Start an inference service>`.
KServe is an open-source project for model serving in production on Kubernetes.
Using this deployment method requires a working Kubernetes cluster with KServe installed.

Development
-----------

For development, the container for the AMD Inference server contains the build-time dependencies needed to compile and test the inference server.
However, the development container, also referred to as the dev container, does not contain the inference server source code.
The expected workflow is that you mount the source code into the container.

For development, your host needs Git, Python3, and `Docker <https://docs.docker.com/get-docker/>`__.
Some tests require `Docker-Compose <https://docs.docker.com/compose/install/>`__ as well.

.. code-block:: bash

    git clone https://github.com/Xilinx/inference-server.git

Tests and examples need assets like images and videos to run.
Some of these files are stored in `Git LFS <https://git-lfs.github.com/>`__.
Depending on your host, these files may be automatically downloaded with the ``git clone``.
If some of the files in ``tests/assets`` are very small (less than 300 bytes), then you haven't downloaded these Git LFS artifacts.
From your host or after entering the dev container, use ``git lfs pull`` to get these files.

Build or get the Docker image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use ``docker pull`` to get the dev container from a Docker registry if it's already built.

To build from source, you will need Docker 18.09 or newer because you need to enable `BuildKit <https://docs.docker.com/build/>`__ to build the image.
After cloning the repository, enter the directory and run:

.. code-block:: console

    $ python3 docker/generate.py
    $ ./amdinfer dockerize <platform flags>

The ``generate.py`` script is used to create a dockerfile in the root directory, which is then used by the ``dockerize`` command.
Use ``--help`` to see configurable options for the ``generate.py`` script.
If you want to enable any platforms, pass the appropriate flags.
Look at the platform-specific documentation for more information about these flags.

By default, this builds the dev image as ``<username>/amdinfer-dev:latest``.
After the image is built, run the container:

.. code-block:: console

    $ ./amdinfer run --dev

This command runs the ``<username>/amdinfer-dev:latest`` image, which corresponds to the latest local dev image.
The ``--dev`` preset will mount the working directory into :file:`/workspace/amdinfer/`, mount some additional directories into the container, expose some ports to the host and pass in any available hardware like FPGAs.
Some options may be overridden on the command-line (use :option:`--help` to see the options).
By default, it will open a Bash shell in this container and show you a splash screen to show that you've entered the container.

Compiling the AMD Inference Server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These commands are all run inside the dev container.
Here, :file:`./amdinfer` is aliased to :command:`amdinfer`.

.. code-block:: console

    $ amdinfer build

The build command builds the :program:`amdinfer-server` executable.
By default, this will be the debug version.
You can pass flags to ``build`` to control the compile options.

.. tip::

    When starting new containers or switching to different ones after having run build once, you may need to run ``amdinfer build --regen --clean`` initially.
    New containers mount the working directory and so stale artifacts from previous builds may be present.
    These two flags delete the CMake cache and do a clean build, respectively.

.. warning::

    In general, you should not use ``sudo`` to run ``amdinfer`` commands.
    Some commands create files in your working directory and using ``sudo`` creates files with mixed permissions in your container and host and will even fail in some cases.

The ``build`` will also install the server's Python library in the dev container.
You can use it from Python in the container after importing it.

.. code-block:: python

    import amdinfer

Getting test artifacts
^^^^^^^^^^^^^^^^^^^^^^

For running tests and certain examples, you need to get models and other files.
Make sure you have `Git LFS <https://git-lfs.github.com/>`__ installed.
You can download all files, as shown below with the ``--all`` flag, or download platform-specific files.
Use ``--help`` to see the options available.

.. code-block:: console

    $ git lfs pull
    $ amdinfer get --all

You must abide by the license agreements of these files, if you choose to download them.

Running the AMD Inference Server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once the server is built, start the server to begin serving requests.

.. code-block:: bash

    # start the server
    amdinfer start

    # this command will block and the server will idle for requests
    # from a new terminal, you can send it requests

    # test that the server is ready. The server returns status 200 OK on success
    curl -I http://localhost:8998/v2/health/ready

    # the server can now accept requests over REST/gRPC

    # shutdown the server using Ctrl+C

The :ref:`REST endpoints <rest:REST Endpoints>` available to the server are based on `KServe's v2 specification <https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md>`__.
While using REST directly works, the Python API is an easier way to communicate with the server.

You can also try running the test suite.
The suite is run using PyTest and you can optionally pass Pytest options to the command to filter and choose which tests to run.
Make sure you have the relevant test artifacts as described in the previous section.

.. code-block:: bash

    # this will start the server and test the REST API from Python.
    amdinfer test

Now that we can build and run the server, we will take a look at how to send requests to it using the Python API and link custom applications to the AMD Inference Server using the C++ API.
