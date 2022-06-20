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

.. _AKS:

AKS
===

':github:`AKS <Xilinx/Vitis-AI/tree/v2.5/src/AKS>`' can be used in Xilinx Inference Server for end-to-end inference.

Introduction to AKS
-------------------

Using AKS requires three things: the AKS backend, a graph to run, and kernels.
The AKS backend in installed in the Xilinx Inference Server container and the repository contains sample graphs and kernels that may be used.
This reference material is available in ``external/aks/reference/``
Graphs are defined in JSON and represent an acyclic graph of computation using kernels.
Kernels perform the work and are defined in two parts: the kernel source and the kernel definition.
The source provides the kernels implementation while the definition is a JSON file that provides the metadata for the AKS backend.

Using AKS in Xilinx Inference Server
------------------------------------

Xilinx Inference Server already uses AKS in workers such as ``aks_detect.cpp`` and ``resnet50.cpp``.
These workers are good examples of how to incorporate AKS into a Xilinx Inference Server worker.
The worker needs to pass a path to an AKS graph and a graph to run to the AKS backend.
This information may be hard-coded into the worker or provided by the user at load time.

When a request is received by a worker that uses AKS, it must eventually enqueue the job to AKS's manager and then parse the response.
This response may vary depending on how the AKS graph that is being executed is defined as the last kernel in the graph determines the output format.
Thus, unless the response can be generalized, you may need a worker per AKS graph.

To use AKS with a new workload, first define any new kernels that you need.
Then, you can write a graph to describe the desired dataflow.
Refer to AKS's documentation for more information about these steps.
Next, you can write a worker to handle this graph or reuse an existing worker if the graph's ouptut is compatible.
More information about workers in Xilinx Inference Server is in :ref:`Architecture <architectureWorkers>`.
