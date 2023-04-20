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

amdinfer
--------

Depending on the context, *amdinfer* can be used to refer to different things.
If the usage is ambiguous, it will be qualified with the appropriate word to clarify the meaning:

* *amdinfer* script: refers to the Python script named ``amdinfer`` in the root of the repository that is used for building and starting containers as well as starting the server and running tests.
* *amdinfer* namespace: refers to the C++ namespace used in the source code
* *amdinfer* package: refers to the Python package with the Python client library for the inference server. This package is installable from ``pip``.

Types of Docker images
----------------------

There are two types of Docker images that are used in this project: *development* and *deployment*.

.. _terminology~Development:

Development
^^^^^^^^^^^

Development images contain both the build-time and run-time dependencies of the server executable.
They also contains other tools such as linting tools to help the development process.
These images are used to compile and test the server executable using the tests in the repository.
They are not pre-built and must be built by the user using the provided scripts.
The development images do not contain any inference server source code.
Instead, the standard workflow to use development images is to clone the inference server repository, build a development image and start a container using the provided script that will mount the working directory into the container.
Under the standard naming convention used by the *amdinfer* script during the build, this image will be tagged as ``$(whoami)/amdinfer-dev:latest``.

.. _terminology~Deployment:

Deployment
^^^^^^^^^^

Deployment images contain only the the compiled server executable and its run-time dependencies.
They are optimized for size and used to deploy the server locally, on Kubernetes or on KServe.
By default, as deployment images start, they start the server executable automatically with the default arguments.
You can override these values by setting your own when you start a container from this image.
The published images for the AMD Inference Server on `Docker Hub <LinkInferenceServerDockerHub_>`_ are deployment images.
Under the standard naming convention used by the *amdinfer* script during the build, this image will be tagged as ``$(whoami)/amdinfer:latest``.

Types of users
--------------

Different *users* of the AMD Inference Server have different needs.
There are three overlapping sets of users for the inference server: *clients*, *administrators* and *developers*.
Where appropriate, the documentation will note the intended audience of the content.

Clients
^^^^^^^

Clients interact with the server using its APIs to send it inference requests.
The server also provides additional methods to check its status or that of the running models that clients can use.
Clients can talk to the server over any of the protocols that the server exposes for communication such as HTTP/REST or gRPC.
Using the server's C++ API, clients can write custom applications that use the server backend directly.
The easiest way for clients to communicate to the server is using one of the provided client libraries but they can also use the supported protocols directly.

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

Developers
^^^^^^^^^^

Developers tinker with the server executable itself by compiling it from source.
Building the server from source requires using a development container.
In the development container, developers can build the server executable and run tests using the executable directly.
The AMD Inference Server uses CMake to build the executables and tests.
After testing, developers build the deployment images containing the server executable, backends, and the needed run-time dependencies using the *amdinfer* script.
