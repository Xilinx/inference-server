..
    Copyright 2024 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

rocAL
========

The rocAL backend supports full processing pipeline for data_loading, meta-data loading, augmentations, 
and data-format conversions for training and inference on CPU or AMD GPU.
For more details about rocAL, please visit `rocAL <https://github.com/ROCm/rocAL/tree/develop/docs>`__.

Model support
-------------

The rocAL backend supports models in .json format.
Users can define their own pipeline using the following format:
"pipeline": Defines a rocAL pipeline which consists of one or more operations to be executed.
"operation": Defines a rocAL operation to be executed. Each operation has its own parameters. See rocAL Operation section for details.

example .json file that runs resize operation followed by crop, mirror, normalization operations:

{
{

  "pipeline": [
    {"operation": "Resize", "dest_width": 224, "dest_height": 224, "is_output": false},

    {"operation": "CropMirrorNormalize", "crop_depth": 224, "crop_height": 224, "crop_width": 224, "start_x": 0, "start_y": 0, "start_z": 0,

    "mean": [0.485, 0.456, 0.406], "std_dev": [0.229, 0.224, 0.225], "is_output": true}

  ]

}


Hardware support
----------------

For supported hardwards, please visit `ROCm supported hardward <https://github.com/ROCm/rocAL?tab=readme-ov-file#tested-configurations>`__.
rocAL has been tested and verified on `Tested Configurations <https://github.com/ROCm/rocAL?tab=readme-ov-file#tested-configurations>`__.

Host setup
----------

Follow the `ROCm installation instructions <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/tutorial/quick-start.html`__ to install ROCm on your host machine.
Your ROCm version should ideally match the version installed in the inference server container but mismatching versions may work as well in some cases.
Ensure you can detect the GPUs on your host machine with ``/opt/rocm/bin/rocminfo``.

Build an image
--------------

To build an image with rocAL enabled, you need to add the ``--rocal`` to the ``amdinfer dockerize`` command:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the development image $(whoami)/amdinfer-dev:latest
    ./amdinfer dockerize --rocal

    # build the development image $(whoami)/amdinfer-dev-migraphx:latest
    ./amdinfer dockerize --rocal --suffix="-rocal"

    # build the deployment image $(whoami)/amdinfer-migraphx:latest
    ./amdinfer dockerize --rocal --suffix="-rocal" --production

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

    $ ./amdinfer get --rocal --all-models

Loading the backend
-------------------

.. include:: /dry.rst
    :start-after: +loading_the_backend_intro
    :end-before: -loading_the_backend_intro

.. tabs::

    .. code-tab:: c++ C++

        // amdinfer::Client* client;
        // amdinfer::ParameterMap parameters;
        std::string endpoint = client->workerLoad("rocal", parameters)

    .. code-tab:: python Python

        # client = amdinfer.Client()
        # parameters = amdinfer.ParameterMap()
        endpoint = client.workerLoad("rocal", parameters)

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

Troubleshooting
---------------

If you run into problems, first check the :ref:`general troubleshooting guide <troubleshooting:Troubleshooting>` guide.
Then continue on to this XModel specific troubleshooting guide.
You will need access to the machine where the inference server is running to debug.