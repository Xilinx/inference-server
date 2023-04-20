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

..
    The contents of this file are inserted at the end of every RST source file

.. _LinkDockerInstallLinux: https://docs.docker.com/engine/install/
.. _LinkDockerRun: https://docs.docker.com/engine/reference/commandline/run/
.. _LinkInferenceServerDockerHub: https://hub.docker.com/r/amdih/serve
.. _LinkInferenceServerRepository: https://github.com/Xilinx/inference-server
.. _LinkInferenceServerDocumentation: https://xilinx.github.io/inference-server/
.. _LinkZenDNN: https://www.amd.com/en/developer/zendnn.html
.. _LinkZenDNNdownload: https://www.amd.com/en/developer/zendnn.html#downloads
.. _LinkZenDNNguide: https://www.amd.com/en/developer/zendnn.html#documentation
.. _LinkZenDNNptGuide: https://www.amd.com/content/dam/amd/en/documents/developer/pytorch-zendnn-user-guide-4.0.pdf
.. _LinkZenDNNtfGuide: https://www.amd.com/content/dam/amd/en/documents/developer/tensorflow-zendnn-user-guide-4.0.pdf


.. |define_deployment| replace:: the act of making the AMD Inference Server available to respond to inference requests from clients

.. |define_ensemble| replace:: a logical pipeline of workers to execute a graph of computations where the output tensors of one model are passed as input to others

.. |define_model_repository| replace:: a directory that exists on the host machine where the server :term:`container <Container (Docker)>` is running and it holds the models you want to serve and their associated metadata in a standard structure
