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

AMD/Xilinx Inference Server
===========================

<<<<<<< HEAD
The AMD/Xilinx Inference Server is an easy-to-use inferencing solution designed for AMD CPUs, GPUs, and FPGAs.
=======
* enter docker container as proteus-user
./proteus run --dev

* give root a password, so we can do su

sudo passwd root # (enter something easy, like asdf, as password)

* become root
su  # (use the password)


*      start server as root

export LD_LIBRARY_PATH=/workspace/proteus/build/workers:$LD_LIBRARY_PATH
./proteus start &

* now become proteus-user again
su proteus-user

* run the script with the API calls
python3 examples/python/hello_migraphx.py




Xilinx Inference Server
=======================

Xilinx Inference Server is an easy-to-use inferencing solution designed for Xilinx FPGAs and :github:`Vitis AI <Xilinx/Vitis-AI>`.
>>>>>>> 5d6e637 (Update README.rst)
It can be deployed as a server or through custom applications using its C++ API.
The AMD/Xilinx Inference Server can also be extended to support other hardware accelerators and machine learning frameworks.

Features
--------

* Inference Server: The AMD/Xilinx Inference Server supports client requests using HTTP REST / websockets protocols using an API based on `KServe's v2 specification <https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md>`__
* C++ API: custom applications to directly call the backend to bypass the REST interface
* Python REST library: clients can submit requests to the inference server from Python through this simplified Python API
* Efficient hardware usage: The AMD/Xilinx Inference Server will automatically make use of all available FPGAs on a machine as needed with `XRM <https://github.com/Xilinx/XRM>`__
* User-defined model parallelism: users can define how many models, and how many instances of each, to run simultaneously
* Batching: incoming requests are batched based on the model's specifications transparently
* Integrated with Vitis AI: The AMD/Xilinx Inference Server can serve most xmodels generated from Vitis AI
* End-to-end inference: A graph of computation such as pre- and post-processing can be written and deployed with the AMD/Xilinx Inference Server using `AKS <https://github.com/Xilinx/Vitis-AI/tree/master/tools/AKS>`__


Learn more
----------

The documentation for the AMD/Xilinx Inference Server is available `online <https://xilinx.github.io/inference-server/>`__.

Check out the `Quickstart <https://xilinx.github.io/inference-server/main/quickstart.html>`__ on how to get started.

Support
-------

Raise issues if you find a bug or need help.
Refer to `Contributing <https://xilinx.github.io/inference-server/main/contributing.html>`__ for more information.

License
-------

The AMD/Xilinx Inference Server is licensed under the terms of Apache 2.0 (see `LICENSE <https://github.com/Xilinx/inference-server/blob/main/LICENSE>`__).
The LICENSE file contains additional license information for third-party files distributed with this work.
More license information can be seen in the `dependencies <https://xilinx.github.io/inference-server/main/dependencies.html>`__.
