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

ZenDNN for Inference Server
===============================

Xilinx inference server is integrated with
`ZenDNN <https://developer.amd.com/zendnn/>`__ optimized libraries.
Currently, the Xilinx Inference Server supports the ZenDNN optimized for
TensorFlow and PyTorch.

For ZenDNN performance, plese refer to TensorFlow + ZenDNN and
PyTorch + ZenDNN User Guide available at `ZenDNN AMD Developer <https://developer.amd.com/zendnn/>`_ site.

Setup and Build
---------------

To use Inference Server enabled with TensorFlow/PyTorch+ZenDNN, you'll need Git 
and Docker installed on your machine.

1. Download C++ package for TensorFlow/PyTorch+ZenDNN

   1. Go to https://developer.amd.com/zendnn/
   2. Download the file

      1. For TensorFlow: TF_v2.7_ZenDNN_v3.2_C++_API.zip
      2. For PyTorch: PT_v1.9_ZenDNN_v3.2_C++_API.zip

      NOTE: The PyTorch package (PT_v1.9_ZenDNN_v3.2_C++_API.zip)
      is not currently available at https://developer.amd.com/zendnn/ and thus
      the PT+ZenDNN path will be disabled currently. Once the package is available,
      the path will be enabled to use PT+ZenDNN within the Inference Server.

   3. For download, you will be required to sign a EULA. Please read
      through carefully and click on accept to download the package.
   4. Copy the downloaded package within the repository. This package
      will be used for further setup.

2. Build the docker with TensorFlow/PyTorch+ZenDNN

   To build docker image with TensorFlow/PyTorch+ZenDNN, use the command below:

   1. For TensorFlow

      .. code-block:: console

         $ ./proteus dockerize --tfzendnn={path/to/TF_v2.7_ZenDNN_v3.2_C++_API.zip}

   2. For PyTorch

      .. code-block:: console

         $./proteus dockerize --ptzendnn={path/to/TF_v2.7_ZenDNN_v3.2_C++_API.zip}

   This will build a docker image with all the dependencies required for
   the Xilinx Inference Server and setup TensorFlow/PyTorch+ZenDNN within the
   image for further usage.

   NOTE: The downloaded package must be inside the inference-server
   folder since Docker will not be able to access the file outside of
   the repository.

3. Run the container

   By default, the stable dev docker image is built and to run the
   container, use the command:

   .. code-block:: console

      $ ./proteus run --dev

4. Build Xilinx Inference Server

   Now that the environment is setup within the docker container, we
   need to build the Inference Server. The below command will build the
   stable debug build of the Xilinx Inference Server.

   .. code-block:: console

      $ ./proteus build --debug

Get objects
-----------

Run the following command to get some git lfs assets for examples/tests.

.. code-block:: console

   $ git lfs fetch --all
   $ git lfs pull

To run the examples and test cases, we need to download some models.
The below section will walk through on downloading and setting up models.

TensorFlow + ZenDNN
^^^^^^^^^^^^^^^^^^^

Run the command below to download a ResNet50 tensorflow model from
`VitisAI repository <https://github.com/Xilinx/Vitis-AI/blob/master/models/AI-Model-Zoo/model-list/tf_resnetv1_50_imagenet_224_224_6.97G_2.0/model.yaml>`__

.. code-block:: console

   $ ./proteus get --tfzendnn

The model downloaded will be available at :code:`./external/tensorflow_models.`


PyTorch + ZenDNN
^^^^^^^^^^^^^^^^

Run the command below to download a ResNet50 tensorflow model from
`VitisAI repository <https://github.com/Xilinx/Vitis-AI/blob/master/models/AI-Model-Zoo/model-list/tf_resnetv1_50_imagenet_224_224_6.97G_2.0/model.yaml>`__

.. code-block:: console

   $ ./proteus get --ptzendnn

The model downloaded will be available at :code:`./external/pytorch_models`.
We need to convert the downloaded PyTorch eager model to TorchScript 
Model (`Exporting to TorchScript docs <https://pytorch.org/tutorials/advanced/cpp_export.html#converting-to-torch-script-via-tracing>`_).

To convert the model to TorchScript model, follow the steps.

1. We will need to use the PyTorch python API. Install requirements with:

   .. code-block:: console
      
      $ pip3 install -r tools/zendnn/requirements.txt

2. To convert the model to TorchScript Model do:

   .. code-block:: console

      $ python tools/zendnn/convert_to_torchscript.py --graph external/pytorch_models/resnet50_pretrained.pth

   The script will do the following:

   1. Load ResNet50 architecture from tools/zendnn/resnet50.py file.
   2. Load the downloaded weights to the model.
   3. Do a jit trace of model.
   4. Save the traced TorchScript model to the same location with .pt extension.

The converted TorchScript model will be used by the examples and tests.
For more info on TorchScript models, please visit `PyTorch docs <https://pytorch.org/tutorials/advanced/cpp_export.html>`_.

Examples
--------

There are two examples provided in the repo (Python API and C++ API) for both TensorFlow and PyTorch.

Python API
^^^^^^^^^^

Python examples below will do the following:

1. Start the Xilinx Inference Server on HTTP port 8998
2. Load the Xilinx Inference Server with the specified model file
3. Read the image specified / Create dummy data
4. Sends the data to the Xilinx Inference Server over HTTP
5. Get the result back from the Xilinx Inference Server over HTTP
6. Post process if any and display the output

TensorFlow + ZenDNN
~~~~~~~~~~~~~~~~~~~

The python example is available at :code:`examples/python/tf_zendnn.py`.

1. To run the example with a real image:

   .. code-block:: console

      $ python examples/python/tf_zendnn.py --graph ./external/tensorflow_models/resnet_v1_50_inference.pb --image_location ./tests/assets/dog-3619020_640.jpg

2. To run the example with dummy data:

   .. code-block:: console

      $ python examples/python/tf_zendnn.py --graph ./external/tensorflow_models/resnet_v1_50_inference.pb --batch_size 16 --steps 4

   The above command will run the example with dummy data (4 requests
   with 16 dummy images each). This can be used as a functional test.

For more options, check the help with:

   .. code-block:: console

      $ python examples/python/tf_zendnn.py --help


PyTorch + ZenDNN
~~~~~~~~~~~~~~~~

The python example is available at :code:`examples/python/pt_zendnn.py`.

1. To run the example with a real image:

   .. code-block:: console

      $ python examples/python/pt_zendnn.py --graph ./external/pytorch_models/resnet50_pretrained.pt --image_location ./tests/assets/dog-3619020_640.jpg

2. To run the example with dummy data:

   .. code-block:: console

      $ python examples/python/pt_zendnn.py --graph ./external/pytorch_models/resnet50_pretrained.pt --batch_size 16 --steps 4

   The above command will run the example with dummy data (4 requests
   with 16 dummy images each). This can be used as a functional test.

For more options, check the help with:

   .. code-block:: console

      $ python examples/python/pt_zendnn.py --help


C++ API
^^^^^^^

The C++ API bypasses the HTTP server and connects directly to the
Inference Server. The flow is as follows 

   1. Load the Xilinx Inference Server with the specified model file
   2. Read the image specified / Create dummy data and prepare input
   3. The data is packed into an Interface object and pushed to a queue
   4. Retrieve the result back from the Xilinx Inference Server
   5. Post process if any and display the output

The C++ example will be built when the server is being built according to the packages available.

* To run the C++ example with real image, provide :code:`image_location` in :code:`Option` struct.
* If :code:`image_location` is set to :code:`""`, dummy data will be used. This can be used for benchmarking.

TensorFlow + ZenDNN
~~~~~~~~~~~~~~~~~~~

Source is available at :code:`examples/cpp/tf_zendnn_client.cpp`. To build and run the example:

.. code-block:: console

   $ ./proteus build --debug && ./build/Debug/examples/cpp/tf_zendnn_client

PyTorch + ZenDNN
~~~~~~~~~~~~~~~~

Source is available at :code:`examples/cpp/pt_zendnn_client.cpp`. To build and run the example:

.. code-block:: console

   $ ./proteus build --debug && ./build/Debug/examples/cpp/pt_zendnn_client


Run Tests
---------

To verify the working of TensorFlow+ZenDNN in Xilinx Inference
Server, run a sample test case. This test will load a model and run
with a sample image and assert the output.

1. For TensorFlow + ZenDNN

   .. code-block:: console

      $ ./proteus test -k tfzendnn

2. For PyTorch + ZenDNN

   .. code-block:: console

      $ ./proteus test -k ptzendnn
