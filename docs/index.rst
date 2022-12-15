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

Welcome to the official documentation for the AMD Inference Server: an open-source tool to deploy your machine learning models and make them accessible to clients for inference.

If you are new to the project, start with the :ref:`Introduction <introduction:introduction>` to get an overview of what it's about and how this documentation is organized.

Use the sidebar to navigate through the different pages in the documentation.
Note that the documentation is versioned.
By default, it shows the latest documentation corresponding to the current state of the `code <https://github.com/Xilinx/inference-server>`__.
To see other versions, you can use "Read the Docs" panel on the bottom-left, visit this `landing page <https://xilinx.github.io/inference-server/>`__ or edit the version in the URL directly.

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: About

    introduction
    dependencies
    roadmap
    changelog

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: Quickstart

    Inference <quickstart_inference>
    Deployment <quickstart_deployment>
    Development <quickstart_development>

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: Libraries and API

    cpp_user_api
    python
    rest
    cli

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: Examples

    hello_world_echo
    example_resnet50_cpp
    example_resnet50_python

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: Using the Server

    platforms
    Deploying with Docker <docker>
    Deploying with KServe <kserve>
    performance_factors

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: Developers

    contributing
    architecture
    aks
    logging
    benchmarking
    metrics
    tracing
    cpp_api/cpp_root
