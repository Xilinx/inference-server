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

ZenDNN
======

AMD Inference Server is integrated with `ZenDNN <ZenDNN_>`_ optimized libraries for running inference on AMD EPYC processors with TensorFlow and PyTorch.
Check the :ref:`examples <zendnn_examples>` to see how to use ZenDNN with the Inference Server.

Build an image
--------------

To build an image with TensorFlow/Pytorch + ZenDNN enabled, you need to download the ZenDNN libraries and then build the image by pointing the build script to the location of these downloaded packages.

Download the ZenDNN packages
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can download the TF+ZenDNN and/or PT+ZenDNN packages from the `ZenDNN developer downloads <ZenDNN_download_>`_.
Before downloading these packages, you will be required to read and agree to the End User License Agreement (EULA).

1. For TensorFlow: download ``TF_v2.9_ZenDNN_v3.3_C++_API.zip``
2. For PyTorch: download ``PT_v1.11.0_ZenDNN_v3.3_C++_API.zip``

You can download one or both, depending on what you want to enable.

Build the image
^^^^^^^^^^^^^^^

After downloading the packages, place them in the root of the repository.
To build an image with ZenDNN enabled, you need to add the the appropriate flag(s) to the ``proteus dockerize`` command:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the dev image $(whoami)/proteus-dev-zendnn:latest
    ./proteus dockerize --tfzendnn=./TF_v2.9_ZenDNN_v3.3_C++_API.zip --ptzendnn=./PT_v1.11.0_ZenDNN_v3.3_C++_API.zip --suffix="-zendnn"

    # build the production image $(whoami)/proteus-zendnn:latest
    ./proteus dockerize --tfzendnn=./TF_v2.9_ZenDNN_v3.3_C++_API.zip --ptzendnn=./PT_v1.11.0_ZenDNN_v3.3_C++_API.zip --suffix="-zendnn" --production

You can choose to build a image with just TF+ZenDNN or just PT+ZenDNN by only passing the appropriate flag.

.. note::

    The downloaded ZenDNN packages will be used by the Docker build process so they must be in the inference server repository directory and in a location that is not excluded by the ``.dockerignore`` file.
    These instructions suggest using the repository root but any path that meets this criteria will work.

Get assets and models
---------------------

You can download the assets and models used for tests and examples with:

.. code-block:: console

   $ ./proteus get --tfzendnn --ptzendnn

Freezing PyTorch models
-----------------------

To use with the AMD Inference Server, you need to `convert downloaded PyTorch Eager models to TorchScript <https://pytorch.org/tutorials/advanced/cpp_export.html#step-1-converting-your-pytorch-model-to-torch-script>`_.

Run Tests
---------

To verify the working of TensorFlow+ZenDNN in the AMD Inference Server, run a sample test case. This test will load a model and run with a sample image and assert the output.

1. For TensorFlow + ZenDNN

   .. code-block:: console

      $ ./proteus test -k tfzendnn

2. For PyTorch + ZenDNN

   .. code-block:: console

      $ ./proteus test -k ptzendnn

Tune performance
----------------

For tuning ZenDNN performance, you can refer to the TensorFlow + ZenDNN and PyTorch + ZenDNN `user guides <ZenDNN_guide_>`_.
