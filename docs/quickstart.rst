..
    Copyright 2021 Xilinx Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

.. _quickstart:

Quickstart
==========

The easiest way to use the AMD Inference Server is to run it inside a Docker container.
For these instructions, you'll need Git, Python3, and `Docker <https://docs.docker.com/get-docker/>`__.
Also ensure that you set up your host appropriately depending on which platform(s) you are using (e.g. :ref:`Vitis AI <vitis_ai:vitis ai>`).
The helper script used for most of the commands here is :file:`proteus`: a Python script with many helpful options.
The most up-to-date documentation for this script can be seen with :ref:`online <cli:command-line interface>` or on the terminal with :option:`--help`.
You can also use :option:`--dry-run` before any command to see the underlying commands the script is running.

Some options also require `Docker-Compose <https://docs.docker.com/compose/install/>`__.
Running tests requires using Git LFS when cloning this repository to get testing assets.

Build or Get the Docker Image
-----------------------------

We can build several types of containers.
Development (dev) containers are intended for working on the AMD Inference Server or applications that link to the AMD Inference Server. They include all the build dependencies and mount the working directory into the container and drop the user into a terminal when they start.
Production containers only contain the runtime dependencies of the :program:`proteus-server` executable and automatically run the executable when they start.
The Docker image can also be built with various options to disable certain components.

Currently, these images are not pre-built anywhere and so must be built by the user.
We require `BuildKit <https://docs.docker.com/develop/develop-images/build_enhancements/>`__ to build the image so your Docker version should be at least 18.09.

.. code-block:: console

    $ ./proteus dockerize

By default, this builds the stable dev image as ``{username}/proteus-dev``.
After the image is built, run the container:

.. code-block:: console

    $ ./proteus run --dev

This command runs the ``{username}/proteus-dev:latest`` image, which corresponds to the latest local dev image.
The dev container will mount the working directory into :file:`/workspace/proteus/`, mount some additional directories into the container, expose some ports to the host and pass in any available hardware like FPGAs.
These details are the :option:`--dev` profile.
Some options may be overridden on the command-line (use :option:`--help` to see the options).
If you'd like to enable :ref:`TF+ZenDNN or PT+ZenDNN <zendnn:ZenDNN>` support in the container, pass the appropriate flag to the :option:`dockerize` command.

Building the AMD Inference Server
---------------------------------

These commands are all run inside the dev container.
Here, :file:`./proteus` is aliased to :command:`proteus`.
Note, in general, you should not use ``sudo`` to run ``proteus`` commands.

.. code-block:: console

    $ proteus build --all

The build command builds :program:`proteus-server` as well as the AKS kernels and documentation.
By default, this will be the debug version.

AKS is the :ref:`AI Kernel Scheduler <AKS>` that may be used in the AMD Inference Server.
The AKS kernels need to be built prior to starting the server and requesting inferences from a worker that uses AKS.
Subsequent builds can omit :option:`--all` to skip rebuilding the AKS kernels.

.. attention:: When starting new containers or switching to different ones after having run build once, you may need to run ``proteus build --regen --clean`` initially. New containers mount the working directory and so stale artifacts from previous builds may be present. These two flags delete the CMake cache and do a clean build, respectively.

Getting Artifacts
-----------------

For running tests and certain examples, you may need to get artifacts such as test images and XModels.

.. code-block:: console

    $ proteus get

You must abide by the license agreements of these files, if you choose to download them.

Running the AMD Inference Server
--------------------------------

Once the server is built, start the server to begin serving requests.

.. code-block:: bash

    # start proteus-server in the background
    $ proteus start &

    # test that the server is ready. The server returns status 200 OK on success
    $ curl -I http://localhost:8998/v2/health/ready

    # the server can now accept requests over REST/gRPC

    # shutdown the server
    $ kill -2 $(pidof proteus-server)

You can also try running the test suite.
You may need to get testing artifacts (see above) and have cloned the repository with Git LFS enabled.

.. code-block:: bash

    # this will start the server and test the REST API from Python.
    $ proteus test

Now that we can build and run the server, we will take a look at how to send requests to it using the Python API and link custom applications to the AMD Inference Server using the C++ API.
