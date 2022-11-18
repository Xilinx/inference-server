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

FPGAs - Vitis AI
================

Using the AMD Inference Server with Vitis AI and FPGAs requires some additional setup prior to use.

Set up the host and FPGAs
-------------------------

The details for setting up your host are available online in the :github:`Vitis AI <Xilinx/Vitis-AI/tree/master/setup>` repository.
These instructions will depend on whether you are using :github:`Alveo cards <Xilinx/Vitis-AI/tree/master/setup/alveo>` or :github:`VCK5000 <Xilinx/Vitis-AI/tree/master/setup/vck5000>`.
In essence, you will need to install software, program the shell on the FPGA(s) and download XCLBINs.

Software
^^^^^^^^

You need to install the Xilinx Runtime (XRT) to communicate with the FPGA over PCIe.
The XRT version on the host should match the one installed in the container where the server will be running.
The Xilinx Resource Manager (XRM) is not needed on the host because it is already installed in the container.

Shell
^^^^^

The Vitis AI repository contains scripts to install packages and flash the shell on your FPGA.
They use XRT to program the FPGA.
In most cases, flashing the shell will be a one-time act as it is persistent.
However, for some FPGAs such as the Alveo U250, there is an intermediate second shell that must be reprogrammed after every power cycle.
Follow the instructions in the scripts to install the secondary shell.

You can use ``xbutil validate --device <device id>`` to confirm that your FPGA is ready to use.
If this executable is not on your PATH, it should be in ``/opt/xilinx/xrt/bin``.

XCLBINs
^^^^^^^

The XCLBINs define the DPU that will run on the FPGA.
By default, these are installed in ``/opt/xilinx/overlaybins/<DPU>/*``
The environment variable ``XLNX_VART_FIRMWARE`` must be set where the inference server is running to point to the directory where the XCLBINs for your FPGA are.
You can mount this directory in your container to enable it to access these files.
Alternatively, you can also copy XCLBINs directly into the container as well.

Build an image
--------------

To build an image with Vitis AI enabled, you need to add the ``--vitis`` to the ``amdinfer dockerize`` command:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the dev image $(whoami)/amdinfer-dev-vitis:latest
    ./amdinfer dockerize --vitis --suffix="-vitis"

    # build the production image $(whoami)/amdinfer-vitis:latest
    ./amdinfer dockerize --vitis --suffix="-vitis" --production

Get assets and models
---------------------

You can download the assets and models used for tests and examples with:

.. code-block:: console

    $ ./amdinfer get --vitis
