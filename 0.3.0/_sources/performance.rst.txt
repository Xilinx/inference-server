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

wrk Benchmarks
--------------

These tests are run on an unloaded machine with the following specs:

* CPU: 2x Intel(R) Xeon(R) Gold 6252 CPU @ 2.10GHz
* RAM: 12x 8 GB 2666 MHz DDR4

Running GET requests on the v2 endpoint:

.. code-block:: console

    $ ./build/bin/wrk -t32 -c400 -d10s http://127.0.0.1:8501/v2
    Running 10s test @ http://127.0.0.1:8501/v2
        32 threads and 400 connections
        Thread Stats   Avg      Stdev     Max   +/- Stdev
            Latency     1.13ms  321.66us  13.19ms   80.47%
            Req/Sec    10.64k     1.41k   23.01k    82.76%
        3417779 requests in 10.10s, 655.15MB read
    Requests/sec: 338412.42
    Transfer/sec:     64.87MB

Running POST requests on v2/models/echo/infer:

.. code-block:: console

    $ cat tmp.lua
    wrk.method = "POST"
    wrk.body = '{"id": "hello_world", "parameters": {"key3": "value3"}, "inputs": [{"name": "echo", "shape": [1], "datatype": [1], "parameters": {"key": "value"}, "data": [2]}], "outputs": [{"name": "echo", "parameters": {"key2": "value2"}}]}'
    wrk.headers["Content-Type"] = "application/json"
    $ ./build/bin/wrk -t32 -c400 -d10s -s tmp.lua http://127.0.0.1:8501/v2/models/echo/infer
    Running 10s test @ http://127.0.0.1:8501/v2/models/echo/infer
    32 threads and 400 connections
    Thread Stats   Avg      Stdev     Max   +/- Stdev
        Latency    12.56ms    8.09ms 198.78ms   90.32%
        Req/Sec     1.01k   196.68     5.22k    88.09%
    323168 requests in 10.10s, 53.63MB read
    Requests/sec:  32002.39
    Transfer/sec:      5.31MB

These are the results of running AKS resnet50 worker in various configurations.
Note that these tests were run on a relatively weak machine with debug versions
of libraries so these numbers are not absolute.
However, there are some trends that we can see.
With one runner, we seem to top out at around 120 req/s.
We didn't stress four runners all the way but it's likely it would scale linearly.
What's also missing here is context about what AKS by itself can deliver for this graph.

.. code-block:: text

    AMDinfer_Threads wrk_threads wrk_connections wrk_time(s) runners Req/s
    1                4           32              10          1       38.00
    1                8           64              10          1       36.64
    4                4           32              1           1       94.10
    4                4           32              10          1       114.61
    4                4           32              20          1       116.03
    8                4           32              10          1       117.47
    8                8           64              10          1       117.85
    12               4           64              10          1       118.03
    12               4           96              10          1       118.39
    12               8           64              10          1       118.63
    12               8           96              10          1       118.66
    1                4           32              20          4       37.60
    1                8           64              10          4       37.83
    2                4           32              10          4       45.77
    2                8           64              10          4       46.09
    4                4           32              10          4       86.96
    4                8           64              10          4       142.28
    4                8           96              10          4       142.03
    4                12          96              10          4       140.17
    8                4           64              10          4       239.12
    8                4           96              10          4       236.76
    8                8           64              10          4       220.47

In the resnet50 stream case, we observe ~50 FPS per video (i.e. per thread) when collecting from Python.
We have not measured the browser performance.
