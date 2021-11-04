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

Metrics
=======

Xilinx Inference Server exposes metrics using `Prometheus <https://prometheus.io/>`__ and they allow users to check Xilinx Inference Server's state in real-time.

Quickstart
----------

The easiest way to view the collected metrics is to run Prometheus as a binary in the container while an instrumented application runs.
In the container:

.. code-block:: console

    $ cd /tmp
    $ wget https://github.com/prometheus/prometheus/releases/download/v2.30.1/prometheus-2.30.1.linux-amd64.tar.gz
    $ tar -xzf prometheus-2.30.1.linux-amd64.tar.gz
    $ cd /tmp/prometheus-2.30.1.linux-amd64
    $ cp $PROTEUS_ROOT/src/proteus/observation/prometheus.yml .
    $ ./prometheus

By default, Prometheus will use a configuration file named ``prometheus.yml`` in the local directory.
A sample ``prometheus.yml`` is provided in Xilinx Inference Server that can be used as-is or changed as needed.
Documentation about the additional options for this file is available `online <https://prometheus.io/docs/prometheus/latest/configuration/configuration/>`__.

Once the prometheus executable is running, start your instrumented application.
The collected metrics can be viewed, queried and graphed at (by default) ``localhost:9090`` using Prometheus's browser interface.
