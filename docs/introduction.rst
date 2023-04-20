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

Introduction
============

The AMD Inference Server is an open-source tool to deploy your machine learning models and make them accessible to clients for inference.
Out-of-the-box, the server can support selected models that run on AMD CPUs, GPUs or FPGAs by leveraging existing libraries.
For all these models and hardware accelerators, the server presents a common user interface based on community standards so clients can make requests to any using the same API.
The server provides HTTP/REST and gRPC interfaces for clients to submit requests.
For both, there are C++ and Python bindings to simplify writing client programs.
You can also use the server backend directly using the native C++ API to write local applications.

Features
--------

* Supports client requests using HTTP/REST, gRPC and websocket protocols using an API based on `KServe's v2 specification <https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md>`__
* Custom applications can directly call the backend bypassing the other protocols using the native C++ API
* C++ library with Python bindings to simplify making requests to the server
* Incoming requests are transparently batched based on the user specifications
* Users can define how many models, and how many instances of each, to run in parallel
* Users can define a linear chain of models to perform a series of steps such as pre-processing, inference, and post-processing on the server

The AMD Inference Server is integrated with the following libraries out of the gate:

* TensorFlow and PyTorch models with `ZenDNN <LinkZenDNN_>`_ on AMD CPUs
* ONNX models with `MIGraphX <https://github.com/ROCmSoftwarePlatform/AMDMIGraphX>`__ on AMD GPUs
* XModel models with `Vitis AI <https://www.xilinx.com/products/design-tools/vitis/vitis-ai.html>`__ on AMD (Xilinx) FPGAs
* A graph of computation including pre- and post-processing for end-to-end inference with `AKS <https://github.com/Xilinx/Vitis-AI/tree/v2.5/src/AKS>`__ on AMD (Xilinx) FPGAs

Documentation overview
----------------------

The remainder of this documentation is organized as follows:

* The **Getting Started** section continues to talk about the project at a high-level
* The **User Guide** section discusses how to use the server in more depth
* The **Examples** section provides more commentary around the examples in the repository
* The **Developers** section has useful information for developers who are building their own images and compiling the server manually
* The **About** section presents meta information about the inference server itself
* The **Libraries and API** section goes over the different libraries, APIs and tools available to users

Support
-------

This documentation is your best source of support.
You can also raise issues on `Github <https://github.com/Xilinx/inference-server/issues>`__ if you run into a bug or have a question.

The AMD Inference Server is open-source and welcomes contributions.
If you are interested in adding a feature, raise an issue first so your proposal can be discussed.
Follow the :ref:`Contributing <contributing:Contributing>` guidelines when making a new pull request.
