..
    Copyright 2023 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Glossary
--------

.. glossary::

    Administrator (User)
        a :term:`user <User>` that sets up and maintains the inference server deployment container(s)

    Chain
        a linear :term:`ensemble <Ensemble>` where all the output tensors of one stage are inputs to the same next stage without having loops, broadcasts or concatenations

    Client (User)
        a :term:`user <User>` that interacts with a running server using its APIs to send it inference requests

    Container (Docker)
        a standard unit of software that packages up code and all its dependencies so the application runs quickly and reliably from one computing environment to another [1]_

        see also: :term:`image <Image (Docker)>`

    Developer (User)
        a :term:`user <User>` that uses the development container to build and test the server executable

    Ensemble
        |define_ensemble|

    EULA
        End User License Agreement

    Image (Docker)
        a lightweight, standalone, executable package of software that includes everything needed to run an application: code, runtime, system tools, system libraries and settings [1]_

        see also: :term:`containers <Container (Docker)>`

    Model repository
        |define_model_repository|

    User
        anything or anyone that uses the AMD Inference Server i.e. :term:`clients <Client (User)>`, :term:`administrators <Administrator (User)>`, or :term:`developers <Developer (User)>`

    VART
        Vitis AI Runtime: enables applications to use the unified high-level runtime API for both Cloud and Edge

    XRT
        Xilinx Runtime Library: an open-source standardized software interface that facilitates communication between the application code and the accelerated-kernels deployed on the reconfigurable portion of PCIe-based Alveo accelerator cards, Zynq-7000, Zynq UltraScale+ MPSoC based embedded platforms or Versal ACAPs


.. [1] https://www.docker.com/resources/what-container/
