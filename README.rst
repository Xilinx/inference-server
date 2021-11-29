..
    Copyright 2021 Xilinx Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Xilinx Inference Server
=======================

Xilinx Inference Server is an easy-to-use inferencing solution designed for Xilinx FPGAs and `Vitis AI <https://github.com/Xilinx/Vitis-AI>`__.
It can be deployed as a server or through custom applications using its C++ API.
Xilinx Inference Server can also be extended to support other hardware accelerators and machine learning frameworks.

Features
--------

* Inference Server: Xilinx Inference Server supports client requests using HTTP REST / websockets protocols using an API based on `KServe's v2 specification <https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md>`__
* C++ API: custom applications to directly call the backend to bypass the REST interface
* Python REST library: clients can submit requests to the inference server from Python through this simplified Python API
* Efficient hardware usage: Xilinx Inference Server will automatically make use of all available FPGAs on a machine as needed with `XRM <https://github.com/Xilinx/XRM>`__
* User-defined model parallelism: users can define how many models, and how many instances of each, to run simultaneously
* Batching: incoming requests are batched based on the model's specifications transparently
* Integrated with Vitis AI: Xilinx Inference Server can serve most xmodels generated from Vitis AI
* End-to-end inference: A graph of computation such as pre- and post-processing can be written and deployed with Xilinx Inference Server using `AKS <https://github.com/Xilinx/Vitis-AI/tree/master/tools/AKS>`__


Learn more
----------

The documentation for Xilinx Inference Server is available `online <https://expert-funicular-ce3bc1d0.pages.github.io>`__.

Check out the `Quickstart <https://expert-funicular-ce3bc1d0.pages.github.io/quickstart.html>`__ on how to get started.

Support
-------

Raise issues if you find a bug or need help.
Refer to `Contributing <https://expert-funicular-ce3bc1d0.pages.github.io/contributing.html>`__ for more information.

License
-------

Xilinx Inference Server is licensed under the terms of Apache 2.0 (see `LICENSE <https://github.com/Xilinx/inference-server/blob/main/LICENSE>`__).
The LICENSE file contains additional license information for third-party files distributed with this work.
More license information can be seen in the `dependencies <https://expert-funicular-ce3bc1d0.pages.github.io/dependencies.html>`__.