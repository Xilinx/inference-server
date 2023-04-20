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

AKS
===

AKS is the AI Kernel Scheduler and is released as part of Vitis AI.
AMD Inference Server can run a subset of AKS graphs for end-to-end inference with user-defined pre- and post-processing.

AMD Inference Server includes modified versions of the kernels source code, kernel definitions and graph definitions from AKS.
These files are in the ``reference`` directory.
When the ``amdinfer-server`` is built, these reference files are copied into this directory by the build system and excluded from version control.
Users are free to modify these copied files as needed to modify kernels and graphs that are used in AMD Inference Server.

More information about AKS can be found in the `Vitis AI repository <https://github.com/Xilinx/Vitis-AI/tree/v2.5/src/AKS>`_.
