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

Vitis AI
========

Using the AMD Inference Server with Vitis AI and FPGAs requires some additional setup prior to use.

Set up the host and FPGAs
-------------------------

The details for setting up your host are available online in the :github:`Vitis AI <Xilinx/Vitis-AI/tree/2.5/setup>` repository.
These instructions will depend on whether you are using :github:`Alveo cards <Xilinx/Vitis-AI/tree/2.5/setup/alveo>` or :github:`VCK5000 <Xilinx/Vitis-AI/tree/2.5/setup/vck5000>`.
In essence, you will need to install software, program the shell on the FPGA(s) and download XCLBINs.

Software
^^^^^^^^

You need to install :term:`XRT` to communicate with the FPGA over PCIe.
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

Get assets and models
---------------------

You can download the assets and models used for tests and examples with:

.. code-block:: console

    $ ./amdinfer get --vitis --all-models

The AMD Inference Server is using models and XCLBINs from Vitis 2.5 in its tests and examples.
Make sure you have compatible tools, shells and XCLBINs with `Vitis 2.5 <https://github.com/Xilinx/Vitis-AI/tree/v2.5/setup>`__.

Build an image
--------------

To build an image with Vitis AI enabled, you need to add the ``--vitis`` to the ``amdinfer dockerize`` command:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the development image $(whoami)/amdinfer-dev:latest
    ./amdinfer dockerize --vitis

    # build the development image $(whoami)/amdinfer-dev-vitis:latest
    ./amdinfer dockerize --vitis --suffix="-vitis"

    # build the deployment image $(whoami)/amdinfer-vitis:latest
    ./amdinfer dockerize --vitis --suffix="-vitis" --production

Start an image
--------------

The development container can be started with:

.. code-block:: console

    $ ./amdinfer run --dev

This automatically adds the detected devices, publishes ports, and mounts some convenient directories, such as your SSH directory, and drops you into a terminal in the container.
In particular, it will mount the default XCLBIN directory from your host to the container.

You can start the :ref:`deployment container on Docker <docker:start the container>` with something like:

.. code-block:: console

    $ docker run --device /dev/xclmgmt<id> --device /dev/dri [--volume ...]

These ``--device`` flags pass the FPGA to the container and you can mount other directories as needed to make models available.
The name of the ``xclmgmt`` file depends on your host.
The deployment image will need XCLBINs.
There are a few ways to achieve this:

1. Copy XCLBINs into ``./external/overlaybins/*`` before invoking the ``dockerize`` command. The ``Dockerfile`` is configured to copy any files and directories at this path into the deployment image under ``/opt/xilinx/overlaybins``.
2. Copy XCLBINs into the image after building it.
3. Mount the XCLBINs in the container at start time

On Kubernetes, you will need to add the FPGA as a resource to your deployment after installing the `Xilinx FPGA Kubernetes plugin <https://github.com/Xilinx/FPGA_as_a_Service/tree/master/k8s-device-plugin>`__.
The exact name of the resource will depend on which FPGA and shell you are trying to request.

.. code-block:: yaml

    # rest of the deployment logic
    resources:
      limits:
        cpu: "1"
        memory: 2Gi
        xilinx.com/fpga-xilinx_u250_gen3x16_xdma_shell_3_1-0: 1
