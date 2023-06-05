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

Performance
===========

MLCommons
---------

:github:`MLPerf <mlcommons/inference>` is a standard benchmark suite for machine learning inference from `MLCommons <https://mlcommons.org/en/>`__.
The :amdinferTree:`MLCommons application <apps/mlcommons>` uses the ``loadgen`` component of MLPerf to send requests to the inference server for a variety of scenarios and models.

SingleStream
^^^^^^^^^^^^

This scenario sends one inference request after the previous one completes and so measures end-to-end latency.
Note that these tests are not run with same duration/query count as specified in the official MLPerf test rules.

.. tabs::

    .. group-tab:: Fake

        .. chart:: charts/fake_SingleStream_protocols.json

            The Fake model does no work and it shows the latency of the system excluding serialization and inference delays

        .. chart:: charts/fake_SingleStream_protocols_qps.json

            The Fake model does no work and it shows the latency of the system excluding serialization and inference delays

    .. group-tab:: ResNet50

        .. chart:: charts/resnet50_SingleStream_protocols.json

            Using Vitis AI and U250

        .. chart:: charts/resnet50_SingleStream_protocols_qps.json

            Using Vitis AI and U250

Server
^^^^^^

This scenario sends inference requests in a Poisson distribution with a configurable number of outstanding requests.
Note that these tests are not run with same duration/query count as specified in the official MLPerf test rules.

.. tabs::

    .. group-tab:: Fake

        .. chart:: charts/fake_Server_protocols.json

            The Fake model does no work and it shows the latency of the system excluding serialization and inference delays

        .. chart:: charts/fake_Server_protocols_qps.json

            The Fake model does no work and it shows the latency of the system excluding serialization and inference delays
