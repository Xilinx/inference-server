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

This is used as a script, namespace and etc.

Types of Docker images
----------------------

There are two types of Docker images that are used in this project: *development* and *deployment*.

Development
^^^^^^^^^^^

The development image contains both the build-time and run-time dependencies of the server executable.
It also contains other tools such as linting tools to help the development process.
This image is used to compile and test the server executable using the tests in the repository.
It is not pre-built and must be built by the user using the provided scripts.
The standard workflow to use this image is to clone the inference server repository, build the development image and start a container using the provided script that will mount the working directory into the container.
Under the standard naming convention used by inference script

Deployment
^^^^^^^^^^

Types of users
--------------

Different *users* of the AMD Inference Server have different needs.
There are three groups of users for the inference server: *clients*, *administrators* and *developers*, each with increasing levels of responsibility.

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
The AMD Inference Server is deployed with a *deployment container* running on a host machine with Docker.
It can also be deployed on a cluster with Kubernetes or KServe.
The deployment container is an instance of an optimized Docker image containing the server executable and other files needed at run time.
The appropriate Docker image(s) will be provided to administrators for deployment on the target machines.
Choosing the appropriate machine(s) to host the server is important because it determines which *backends* are supported, which in turn determines which models can be used.
Backends define how to execute a model.
For example, the MIGraphX backend uses the MIGraphX library to execute ONNX models on AMD GPUs and it is only usable if the host machine has compatible GPUs.

Administrators may also need to send requests to the inference server they are managing so they are a superset of clients.

Developers
^^^^^^^^^^

Developers tinker with the server executable itself by compiling it from source.
Building the server from source requires using the *development container*:
After testing, developers build the deployment images containing the server executable, backends, and the needed run-time dependencies.
In the development container, the developer can build the server and run tests.
They can also write applications using the inference server's C++ or Python client libraries.


Glossary
--------

.. glossary::

    Deployment
        |define_deployment|

    Users
        Anyone who uses the AMD Inference Server. There are three groups of users: :term:`clients`, :term:`administrators`, or :term:`developers`

    Xilinx Runtime Library
        An open-source standardized software interface that facilitates communication between the application code and the accelerated-kernels deployed on the reconfigurable portion of PCIe-based Alveo accelerator cards, Zynq-7000, Zynq UltraScale+ MPSoC based embedded platforms or Versal ACAPs.

    XRT
        See :term:`Xilinx Runtime Library`
