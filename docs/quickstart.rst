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

The easiest way to use Xilinx Inference Server is to run it inside a Docker container.
For these instructions, you'll need Git, Python3, and `Docker <https://docs.docker.com/get-docker/>`__.

Note: the sequence of instructions in the code blocks below is valid only as written.
If you add or change flags to commands, you may need to change the subsequent commands.
The helper script used for most of the commands here is :file:`proteus`.
The most up-to-date documentation for this script can be seen with :option:`--help`.
To see the actual commands that are run by the script, use :option:`--dry-run`:

.. code-block:: console

    $ ./proteus --dry-run <subcommand> <subcommand options>

Note that the :option:`--dry-run` must be specified before the subcommand!

Some options also require `Docker-Compose <https://docs.docker.com/compose/install/>`__.

Build or Get the Docker Image
-----------------------------

We can build several types of containers:

 * stable vs. nightly: stable containers use the latest released versions of the Vitis-AI libraries that are publicly available. Nightly containers use internal versions of these libraries and thus cannot be built by the general public.
 * development vs. production: dev containers are intended for working on Xilinx Inference Server or applications that link to Xilinx Inference Server. They include all the build dependencies and mount the working directory into the container and drop the user into a terminal when they start. Production containers only contain the runtime dependencies of the :program:`proteus-server` executable and automatically run the executable when they start.

Currently, these images are not pre-built anywhere and so must be built by the user.
You must enable `BuildKit <https://docs.docker.com/develop/develop-images/build_enhancements/>`__ by setting DOCKER_BUILDKIT in the environment, configuring the Docker daemon or using ``docker buildx install`` before attempting to build the image.
Refer to the BuildKit documentation for more information.

.. code-block:: console

    $ ./proteus dockerize

By default, this builds the stable dev image as ``{username}/proteus-dev``.
After the image is built, run the container:

.. code-block:: console

    $ ./proteus run --dev

This command runs the ``{username}/proteus-dev:latest`` image, which corresponds to the latest local dev image.
The dev container will mount the working directory into :file:`/workspace/proteus/`, mount some additional directories into the container, expose some ports to the host and pass in FPGAs.
These details are the :option:`--dev` profile.
Some options may be overridden on the command-line (use :option:`--help` to see the options).

Building Xilinx Inference Server
--------------------------------

These commands are all run inside the dev container.
Here, :file:`./proteus` is aliased to :command:`proteus`.

.. code-block:: console

    $ proteus build --all

The build command builds :program:`proteus-server` as well as the AKS kernels and documentation.
By default, this will be the debug version.

AKS is the :ref:`AI Kernel Scheduler <AKS>` that may be used in Xilinx Inference Server.
The AKS kernels need to be built prior to starting the server and requesting inferences from a worker that uses AKS.
Subsequent builds can omit :option:`--all` to skip rebuilding the AKS kernels.

Getting Artifacts
-----------------

For running tests and certain examples, you may need to get artifacts such as test images and XModels.

.. code-block:: console

    $ proteus get

You must abide by the license agreements of these files, if you choose to download them.

Running Xilinx Inference Server
-------------------------------

Once the server is built, start the server to begin serving requests.

.. code-block:: bash

    # start proteus-server in the background
    $ proteus start &

    # test that the server is ready. The server returns status 200 OK on success
    $ curl -I http://localhost:8998/v2/health/ready

    # the server can now accept requests over REST

    # shutdown the server
    $ kill -2 $(pidof proteus-server)

You can also try running the test suite.

.. code-block:: bash

    # this will start the server and test the REST API from Python.
    $ proteus test

Note: the facedetect test may fail due to a known issue with parsing the XModel.
Ignore any failures with that model at this time.

Now that we can build and run the server, we will take a look at how to send requests to it using the Python API and link custom applications to Xilinx Inference Server using the C++ API.