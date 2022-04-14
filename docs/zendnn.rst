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
TensorFlow only.

TensorFlow + ZenDNN
-------------------

To use Inference Server enabled with TensorFlow+ZenDNN, you'll need Git
and Docker installed on your machine.

1. Download C++ package for TensorFlow+ZenDNN

   1. Go to https://developer.amd.com/zendnn/
   2. Download the file TF_v2.7_ZenDNN_v3.2_C++_API.zip.
   3. For download, you will be required to sign a EULA. Please read
      through carefully and click on accept to download the package.
   4. Copy the downloaded package within the repository. This package
      will be used for further setup.

2. Build the docker with TensorFlow+ZenDNN

   To build docker image with TensorFlow+ZenDNN, use the command below:

   .. code-block:: console

      $ ./proteus dockerize --tfzendnn={path/to/TF_v2.7_ZenDNN_v3.2_C++_API.zip}

   This will build a docker image with all the dependencies required for
   the Xilinx Inference Server and setup TensorFlow+ZenDNN within the
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

5. Get objects

   To run the examples and test cases, we need to download some models.
   Run the command below to download a ResNet50 tensorflow model from
   `here <https://github.com/Xilinx/Vitis-AI/blob/master/models/AI-Model-Zoo/model-list/tf_resnetv1_50_imagenet_224_224_6.97G_2.0/model.yaml>`__

   .. code-block:: console

      $ ./proteus get --tfzendnn

   Also run the following command to get some
   git lfs assets for examples.

   .. code-block:: console

      $ git lfs pull

6. Run tests

   To verify the working of TensorFlow+ZenDNN in Xilinx Inference
   Server, run a sample test case. This test will load a model and run
   with a sample image and assert the output.

   .. code-block:: console

      $ ./proteus test -k tfzendnn

7. Run examples: There are two examples provided in the repo (Python API
   and C++ API).

Examples
~~~~~~~~

There are two examples provided in the repo (Python API and C++ API).

Python API
^^^^^^^^^^

Both the Python examples below will do the following:

   1. Start the Xilinx Inference Server on HTTP port 8998
   2. Load the Xilinx Inference Server with the specified model file
   3. Read the image specified / Create dummy data
   4. Sends the data to the Xilinx Inference Server over HTTP
   5. Get the result back from the Xilinx Inference Server over HTTP
   6. Post process if any and display the output

The python example is available at ``examples/python/tf_zendnn.py``.

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

C++ API
^^^^^^^

The C++ API bypasses the HTTP server and connects directly to the
Inference Server. The flow is as follows 

   1. Load the Xilinx Inference Server with the specified model file
   2. Read the image specified / Create dummy data and prepare input
   3. The data is packed into an Interface object and pushed to a queue
   4. Retrieve the result back from the Xilinx Inference Server
   5. Post process if any and display the output

The C++ example will be built when the server is being built and is
source is available at ``examples/cpp/tf_zendnn_client.cpp``

1. To run the C++ example with real image, provide ``image_location`` in
   ``Option`` struct.
2. If ``image_location`` is set to ``""``, dummy data will be used. This
   can be used for benchmarking.

To build and run the example:

   .. code-block:: console

      $ ./proteus build --debug && ./build/Debug/examples/cpp/tf_zendnn_client
