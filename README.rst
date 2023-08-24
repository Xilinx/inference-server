..
    Copyright 2021 Xilinx, Inc.
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

AMD Inference Server
====================

The AMD Inference Server is an open-source tool to deploy your machine learning models and make them accessible to clients for inference.
Out-of-the-box, the server can support selected models that run on AMD CPUs, GPUs or FPGAs by leveraging existing libraries.
For all these models and hardware accelerators, the server presents a common user interface based on community standards so clients can make requests to any using the same API.
The server provides HTTP/REST and gRPC interfaces for clients to submit requests.
For both, there are C++ and Python bindings to simplify writing client programs.
You can also use the server backend directly using the native C++ API to write local applications.

Features
--------

* Supports client requests using **HTTP/REST, gRPC and websocket protocols** using an API based on `KServe's v2 specification <https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md>`__
* Custom applications can directly call the backend bypassing the other protocols using the **native C++ API**
* **C++ library with Python bindings** to simplify making requests to the server
* Incoming requests are transparently **batched** based on the user specifications
* Users can define how many models, and how many instances of each, to **run in parallel**

The AMD Inference Server is integrated with the following libraries out of the gate:

* TensorFlow and PyTorch models with `ZenDNN <https://developer.amd.com/zendnn/>`__ on CPUs (optimized for AMD CPUs)
* ONNX models with `MIGraphX <https://github.com/ROCmSoftwarePlatform/AMDMIGraphX>`__ on AMD GPUs
* XModel models with `Vitis AI <https://www.xilinx.com/products/design-tools/vitis/vitis-ai.html>`__ on AMD FPGAs
* A graph of computation including as pre- and post-processing can be written using `AKS <https://github.com/Xilinx/Vitis-AI/tree/bbd45838d4a93f894cfc9f232140dc65af2398d1/src/AKS>`__ on AMD FPGAs for end-to-end inference

Quick Start Deployment and Inference
------------------------------------

The following example demonstrates how to deploy the server locally and run a sample inference.
This example runs on the CPU and does not require any special hardware.
You can see a more detailed version of this example in the `quickstart <https://xilinx.github.io/inference-server/main/quickstart.html>`__.

.. code-block:: bash

  # Step 1: Download the example files and create a model repository
  wget https://github.com/Xilinx/inference-server/raw/main/examples/resnet50/quickstart-setup.sh
  chmod +x ./quickstart-setup.sh
  ./quickstart-setup.sh

  # Step 2: Launch the AMD Inference Server
  docker run -d --net=host -v ${PWD}/model_repository:/mnt/models:rw amdih/serve:uif1.1_zendnn_amdinfer_0.3.0 amdinfer-server --enable-repository-watcher

  # Step 3: Install the Python client library
  pip install amdinfer

  # Step 4: Send an inference request
  python3 tfzendnn.py --endpoint resnet50 --image ./dog-3619020_640.jpg --labels ./imagenet_classes.txt

  # Inference should print the following:
  #
  #     Running the TF+ZenDNN example for ResNet50 in Python
  #     Waiting until the server is ready...
  #     Making inferences...
  #     Top 5 classes for ../../tests/assets/dog-3619020_640.jpg:
  #       n02112018 Pomeranian
  #       n02112350 keeshond
  #       n02086079 Pekinese, Pekingese, Peke
  #       n02112137 chow, chow chow
  #       n02113023 Pembroke, Pembroke Welsh corgi

Learn more
----------

The documentation for the AMD Inference Server is available `online <https://xilinx.github.io/inference-server/>`__.

Check out the `quickstart <https://xilinx.github.io/inference-server/main/quickstart.html>`__ online to help you get started.

Support
-------

Raise issues if you find a bug or need help.
Refer to `Contributing <https://xilinx.github.io/inference-server/main/contributing.html>`__ for more information.

License
-------

The AMD Inference Server is licensed under the terms of Apache 2.0 (see `LICENSE <https://github.com/Xilinx/inference-server/blob/main/LICENSE>`__).
The LICENSE file contains additional license information for third-party files distributed with this work.
More license information can be seen in the `dependencies <https://xilinx.github.io/inference-server/main/dependencies.html>`__.
