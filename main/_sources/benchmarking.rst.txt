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

Benchmarking
============

AMD Inference Server can be benchmarked from the command-line using the benchmarking utility.

.. code-block:: console

    $ amdinfer benchmark

This command calls the benchmarking Python script in ``tools/``, which will read ``benchmark.yml`` for the configuration and run the configurations specified.
Each configuration is run a number of times and the results are analyzed and printed to the terminal.
The raw data is also saved in JSON format.

There are multiple kinds of benchmarks that may be run:

*  pytest: use pytest-benchmark to run Python-based benchmarks
*  wrk: use ':github:`wrk <wg/wrk>`' for efficient benchmarking for HTTP/REST tests. These tests require running pytest for these tests first to generate the information needed by ``wrk``.
*  cpp: run C++ executables

Refer to ``benchmark.yml`` for more a detailed explanation of the different options for each type of benchmark.

XModel Benchmarking
-------------------

The XModel test in ``tests/cpp/native/xmodel.cpp`` is the easiest way to benchmark an arbitrary XModel that you may want to serve with AMD Inference Server.
It gets built as part of the regular AMD Inference Server build flow (for benchmarking, use the ``--release`` flag to build optimized executables).
This test accepts a number of arguments (use ``--help`` to see all the options) and makes requests to AMD Inference Server's backend using C++.
It will print out the :abbr:`QPS (queries per second)` of the requested configuration.

.. code-block:: console

    $ ./build/Release/tests/cpp/native/xmodel -m <path to xmodel>

Kernel Simulation
-----------------

With some modifications, the Xmodel worker can be used as a way to simulate a hypothetical FPGA kernel with a certain load.
Running this kind of test requires some code changes, which are described in ``tools/xmodel_benchmark.sh``.
With this script, you can compile the executable with different configurations and loads and print the averaged results to the terminal.
