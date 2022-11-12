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

Tracing
=======

AMD Inference Server reports request tracing using `Jaeger <https://www.jaegertracing.io/>`__ and they allow users to see how requests move through the system in real-time.

Quickstart
----------

The easiest way to view the collected traces is to run Jaeger Tracing as a binary in the container while an instrumented application runs.
In the container:

.. code-block:: console

    $ cd /tmp
    $ wget https://github.com/jaegertracing/jaeger/releases/download/v1.26.0/jaeger-1.26.0-linux-amd64.tar.gz
    $ tar -xzf jaeger-1.26.0-linux-amd64.tar.gz
    $ cd jaeger-1.26.0-linux-amd64
    $ ./jaeger-all-in-one

Once the jaeger executable is running, start your instrumented application.
The collected traces can be viewed at (by default) ``localhost:16686`` using Jaeger's browser interface.
