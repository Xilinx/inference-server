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

Quickstart - Deployment
=======================

This quickstart is intended for a user who is deploying the inference server and configuring the available models for inference.
There are multiple ways to deploy the server but this quickstart only covers a basic Docker-based deployment.
Once the server is up, you and any clients can :ref:`make requests to it <quickstart_inference:Quickstart - Inference>`.

Prerequisites
-------------

* `Docker <https://docs.docker.com/get-docker/>`__
* Network access to enable clients to access the server
* Sufficient disk space to host your models

Prepare the model repository
----------------------------

As the server administrator, you need to identify which models you want to make available for inference and organize them into a model repository.
The model repository is a directory on your host machine where the server is running that will hold your models and their associated metadata.
The format of the directory is as follows:

.. code-block:: text

    /
    ├─ model_a/
    │  ├─ 1/
    │  │  ├─ saved_model.x
    │  ├─ config.pbtxt
    | model_b/
    |  ...

The model name, ``model_a`` in this template, must be unique among the models loaded on a particular server.
This name is used to name the endpoint used to make inference requests to.
Under this directory, there must be a directory named ``1/`` containing the model file itself and a text file named ``config.pbtxt``.
The model file must be named ``saved_model`` and the file extension depends on the type of the model.
The ``config.pbtxt`` file contains metadata for the model.
Consider this example of an MNIST TensorFlow model:

.. code-block:: text

    name: "mnist"
    platform: "tensorflow_graphdef"
    inputs [
      {
        name: "images_in"
        datatype: "FP32"
        shape: [28,28,1]
      }
    ]
    outputs [
      {
        name: "flatten/Reshape"
        datatype: "FP32"
        shape: [10]
      }
    ]

The name must match the name of the model directory.
The platform identifies the type of the model and determines the file extension of the model file.
The supported platforms are:

.. csv-table::
    :header: Platform,Model file extension
    :widths: 90, 10
    :width: 22em

    ``tensorflow_graphdef``,``.pb``
    ``pytorch_torchscript``,``.pt``
    ``vitis_xmodel``,``.xmodel``
    ``onnx_onnxv1``,``.onnx``

The inputs and outputs define the list of input and output tensors for the model.
The names of the tensors may be significant if the platform needs them to perform inference.

Get the deployment image
------------------------

The deployment image is optimized for size and only contains the runtime dependencies of the server to allow for quicker deployments.
It has limited debugging capabilities and it contains a precompiled executable for the server that automatically starts when the container starts.
You can pull the deployment image with Docker if it exists or :ref:`build it yourself <docker:Build the deployment Docker image>`.

Start the image
---------------

You can start a container from the deployment image with ``docker`` as any other image:

.. tabs::

    .. code-tab:: console CPU

        $ docker run -d --volume /path/to/model/repository:/mnt/models:rw --publish 127.0.0.1::8998 --publish 127.0.0.1::50051 <image>

    .. code-tab:: console GPU

        $ docker run -d --device /dev/kfd --device /dev/dri --volume /path/to/model/repository:/mnt/models:rw --publish 127.0.0.1::8998 --publish 127.0.0.1::50051 <image>

    .. code-tab:: console FPGA

        $ docker run -d --device /dev/dri --device /dev/xclmgmt<id> --volume /path/to/model/repository:/mnt/models:rw --publish 127.0.0.1::8998 --publish 127.0.0.1::50051 <image>

.. note::

    These commands are provided as an example for different hardware devices.
    Depending on your particular device(s) or desired container configuration, you may need to add or remove flags.

As the container starts, it will start the server and load the models from your model repository in ``/mnt/models`` in the container.
The ``--publish`` flags will map ports 8998 and 50051 in the container to arbitrary free ports on the host machine for HTTP and gRPC requests, respectively.
You can use ``docker ps`` to show the running containers and what ports on the host machine are used by the container.
Your clients will need these port numbers to make requests to the server.
The endpoints for each model will be the name of the model in the ``config.pbtxt``, which should match the name of the parent directory in the model repository.
Once the container is started, you and any clients can :ref:`make requests to it <quickstart_inference:Quickstart - Inference>`.
