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

If you are new to the project, start with the :ref:`Introduction <introduction:introduction>` and the rest of the **Getting Started** section to get a high-level look at the features, documentation organization and important concepts.

Use the sidebar to navigate through the different pages in the documentation.
Note that the documentation is versioned.
By default, it shows the latest documentation corresponding to the current state of the `code <https://github.com/Xilinx/inference-server>`__.
To see other versions, you can use "Read the Docs" panel on the bottom-left, visit this `landing page <https://xilinx.github.io/inference-server/>`__ or edit the version in the URL directly.

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: Getting started

    introduction
    terminology
    quickstart

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: User guide

    backends
    model_repository
    ensembles
    Deploying with Docker <docker>
    Deploying with KServe <kserve>
    performance_factors
    troubleshooting

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
    :caption: Developers

    quickstart_development
    testing
    architecture
    aks
    logging
    benchmarking
    metrics
    tracing

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: About

    contributing
    dependencies
    changelog
    roadmap

.. toctree::
    :maxdepth: 2
    :hidden:
    :caption: Libraries and API

    amdinfer_script
    cpp_user_api
    python
    rest
    .. cpp_api/cpp_root
