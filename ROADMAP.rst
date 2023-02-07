..
    Copyright 2022 Xilinx, Inc.
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

Roadmap
=======

The AMD Inference Server is in active development and this is the tentative and non-exhaustive roadmap of features we would like to add.
Of course, this is subject to change based on our own assessment and on feedback from the community, both of which may affect which features take priority over others.
More detailed information about the work that's ongoing and/or completed can be found in the :ref:`change log <Changelog>` and the `Github roadmap <https://github.com/Xilinx/inference-server/projects/3>`__.

2022
----

2022 Q1
^^^^^^^

- gRPC support (series of commits starting in :commit:`37a6aad`)

2022 Q2
^^^^^^^

- ZenDNN CPU support (:pr:`17` and :pr:`21`)
- Official integration with KServe (KServe website `#179 <https://github.com/kserve/website/pull/179>`__)

2022 Q3
^^^^^^^

- GPU support (:pr:`34`)

2023
----

The theme for 2023 is ease-of-use and performance.
These two prongs are related and connected as two ways of engaging users and driving development.
Ease-of-use means improving documentation and expanding testing with different models and devices to provide guides on how users can do the same.
Making it easier to install and get started is a big part of that too.
As test coverage expands, the question inevitably gets asked: how is it compared to the alternative?
Thus, measuring performance and reliably reporting results consistently in a reproducible manner becomes important.
The quality of those results should then guide what changes need to be made internally to improve performance.
Having these results to compare with is also useful to maintain the numbers

2023 Q1
^^^^^^^

- Benchmarking with MLPerf

2023 Q2
^^^^^^^

- Refactor memory model
- Enable installation without Docker

2023 Q3
^^^^^^^

- Expanded testing with models in Vitis AI model zoo

2023 Q4
^^^^^^^

Future
------
