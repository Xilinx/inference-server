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

Terminology
===========

This page presents some important terminology to frame the rest of the documentation.

*amdinfer*
----------

Depending on the context, *amdinfer* can be used to refer to different things.
If the usage is ambiguous, it will be qualified with the appropriate word to clarify the meaning:

* *amdinfer* script: refers to the Python script named ``amdinfer`` in the root of the repository that is used for building and starting containers as well as starting the server and running tests.
* *amdinfer* namespace: refers to the C++ namespace used in the source code
* *amdinfer* package: refers to the Python package with the Python client library for the inference server. This package is installable from ``pip``.

Types of Docker images
----------------------

There are two types of Docker images that are used in this project: *development* and *deployment*.

Development
^^^^^^^^^^^

Development images contain both the build-time and run-time dependencies of the server executable.
They also contains other tools such as linting tools to help the development process.
These images are used to compile and test the server executable using the tests in the repository.
They are not pre-built and must be built by the user using the provided scripts.
The development images do not contain any inference server source code.
Instead, the standard workflow to use development images is to clone the inference server repository, build a development image and start a container using the provided script that will mount the working directory into the container.
Under the standard naming convention used by the *amdinfer* script, this image will be tagged as ``$(whoami)/amdinfer-dev:latest``.

Deployment
^^^^^^^^^^

Deployment images contain only the the compiled server executable and its run-time dependencies.
They are optimized for size and used to deploy the server locally, on Kubernetes or on KServe.
By default, as deployment images start, they start the server executable automatically with the default arguments.
You can override these values by setting your own when you start a container from this image.
The published images for the AMD Inference Server on `Docker Hub <InferenceServerDockerHub>`_ are deployment images.

Types of users
--------------

Different *users* of the AMD Inference Server have different needs.
There are three groups of users for the inference server: *clients*, *administrators* and *developers*, each with increasing levels of responsibility.
Where appropriate, the documentation will note the intended audience of the content.

Clients
^^^^^^^

Clients make inference requests to the server but they are not responsible for starting or managing the server.
For clients, the server is already running somewhere and is accessible over a network.
As such, clients must be given an address at which to reach the server as well as endpoints to send requests to.
They use the AMD Inference Server's client libraries to write programs to send inference requests to this server.

Administrators
^^^^^^^^^^^^^^

Administrators *deploy* the server by making it available to respond to inference requests from clients.
They must configure which protocols the server will use, which models are active and how they are configured, and where the server will run.
The AMD Inference Server is deployed with a deployment container running on a host machine with Docker.
It can also be deployed on a cluster with Kubernetes or KServe.
The appropriate Docker image(s) will be provided to administrators for deployment on the target machines.
Choosing the appropriate machine(s) to host the server is important because it determines which *backends* are supported, which in turn determines which models can be used.
Backends define how to execute a model.
For example, the MIGraphX backend uses the MIGraphX library to execute ONNX models on AMD GPUs and it is only usable if the host machine has compatible GPUs.

Administrators may also need to send requests to the inference server they are managing so they are a superset of clients.

Developers
^^^^^^^^^^

Developers tinker with the server executable itself by compiling it from source.
Building the server from source requires using a development container.
In the development container, the developer can build the server executable and run tests using the executable directly.
The AMD Inference Server uses CMake to build the executables and tests.
The build process makes the inference server's C++ and Python libraries available so developers can write custom applications using the API.
After testing, developers build the deployment images containing the server executable, backends, and the needed run-time dependencies using the *amdinfer* script.

Developers are a superset of clients and administrators.
They must be familiar with all stages of building, testing and deploying the inference server.

Glossary
--------

.. glossary::

    Chain
        a linear :term:`ensemble <Ensemble>` where all the output tensors of one stage are inputs to the same next stage without having loops, broadcasts or concatenations

    Container (Docker)
        a standard unit of software that packages up code and all its dependencies so the application runs quickly and reliably from one computing environment to another [1]_

        see also: :term:`image <Image (Docker)>`

    Ensemble
        .. include:: dry.rst
            :start-after: +define_ensembles
            :end-before: -define_ensembles

    Image (Docker)
        a lightweight, standalone, executable package of software that includes everything needed to run an application: code, runtime, system tools, system libraries and settings [1]_

        see also: :term:`containers <Container (Docker)>`

    Model repository
        .. include:: dry.rst
            :start-after: +define_model_repository
            :end-before: -define_model_repository

    Users
        anyone who uses the AMD Inference Server. There are three groups of users: :term:`clients`, :term:`administrators`, or :term:`developers`

    Xilinx Runtime Library
        an open-source standardized software interface that facilitates communication between the application code and the accelerated-kernels deployed on the reconfigurable portion of PCIe-based Alveo accelerator cards, Zynq-7000, Zynq UltraScale+ MPSoC based embedded platforms or Versal ACAPs

    XRT
        see :term:`Xilinx Runtime Library`

.. [1] https://www.docker.com/resources/what-container/
