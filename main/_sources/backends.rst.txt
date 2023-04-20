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

Backends
========

The AMD Inference Server supports a number of backends.
Each backend has its own sets of features and capabilities, which are summarized at a high-level in the table below.
You can see more information about each backend on its own page.

.. csv-table::
    :header: Backend,Hardware,Model Formats,Model Support

    :ref:`CPlusPlus <backends/cplusplus:CPlusPlus>`,CPU \| GPU \| FPGA,.so,✔
    :ref:`MIGraphX <backends/migraphx:MIGraphX>`,GPU,.mxr \| .onnx,✔
    :ref:`PT+ZenDNN <backends/ptzendnn:PtZenDNN>`,CPU,.pt,⚠
    :ref:`TF+ZenDNN <backends/tfzendnn:TfZenDNN>`,CPU,.tf,⚠
    :ref:`Vitis AI <backends/vitis_ai:Vitis AI>`,FPGA,.xmodel,✔

.. toctree::
    :hidden:
    :glob:

    backends/*
