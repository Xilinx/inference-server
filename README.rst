..
    Copyright 2021 Xilinx, Inc.
    Copyright 2022, Advanced Micro Devices, Inc.

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

* TensorFlow and PyTorch models with `ZenDNN <https://developer.amd.com/zendnn/>`__ on AMD CPUs
* ONNX models with `MIGraphX <https://github.com/ROCmSoftwarePlatform/AMDMIGraphX>`__ on AMD GPUs
* XModel models with `Vitis AI <https://www.xilinx.com/products/design-tools/vitis/vitis-ai.html>`__ on AMD FPGAs
* A graph of computation including as pre- and post-processing can be written using `AKS <https://github.com/Xilinx/Vitis-AI/tree/v2.5/src/AKS>`__ on AMD FPGAs for end-to-end inference

Quick Start Inference
----------------------------------------

Quick start requires AMD EPYC CPU::

  # Step 1: Create the example model repository(requires git-lfs)
  git lfs clone https://github.com/Xilinx/inference-server.git
  cd inference-server/examples
  ./fetch_models.sh

  # Step 2: Launch amd-infer server
  docker run -d --net=host -v ${PWD}/model_repository:/mnt/models:rw amdih/serve:uif1.1_zendnn_amdinfer_0.3.0 amdinfer-server --enable-repository-watcher

  # Step 3: Sending inference request
  pip install amdinfer
  cd inference-server/examples/resnet50/
  python3 tfzendnn.py --endpoint resnet50 --image ../../tests/assets/dog-3619020_640.jpg --labels ./imagenet_classes.txt

  # Inference should return the following
  Running the TF+ZenDNN example for ResNet50 in Python
  Waiting until the server is ready...
  Making inferences...
  Top 5 classes for ../../tests/assets/dog-3619020_640.jpg:
    n02112018 Pomeranian
    n02112350 keeshond
    n02086079 Pekinese, Pekingese, Peke
    n02112137 chow, chow chow
    n02113023 Pembroke, Pembroke Welsh corgi

Learn more
----------

The documentation for the AMD Inference Server is available `online <https://xilinx.github.io/inference-server/>`__.

Check out the `Quickstart <https://xilinx.github.io/inference-server/main/quickstart.html>`__ on how to get started.

Support
-------

Raise issues if you find a bug or need help.
Refer to `Contributing <https://xilinx.github.io/inference-server/main/contributing.html>`__ for more information.

License
-------

The AMD Inference Server is licensed under the terms of Apache 2.0 (see `LICENSE <https://github.com/Xilinx/inference-server/blob/main/LICENSE>`__).
The LICENSE file contains additional license information for third-party files distributed with this work.
More license information can be seen in the `dependencies <https://xilinx.github.io/inference-server/main/dependencies.html>`__.
