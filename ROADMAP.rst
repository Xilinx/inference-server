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

2022 Q1
-------

- gRPC support (series of commits starting in :commit:`37a6aad`)

2022 Q2
-------

- ZenDNN CPU support (:pr:`17` and :pr:`21`)
- Official integration with KServe (KServe website `#179 <https://github.com/kserve/website/pull/179>`__)

2022 Q3
-------

- GPU support (:pr:`34`)

Future
------

- Refactor memory model
- Expanded testing with models in Vitis AI model zoo
- Benchmarking with MLPerf
