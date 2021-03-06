..
    Copyright 2022 Xilinx Inc.

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
These instructions assume an Alveo card.
If using a different card, follow the appropriate instructions for your card.

Setting up the host
-------------------

Follow the instructions on :github:`setting up Alveo cards <Xilinx/Vitis-AI/tree/master/setup/alveo>`.
In essence, you will need to program the shell and platform on the FPGA.
You will also need to install XRT.
If you're using Docker, the XRT version on the host should match the one installed in the Docker container.
You also don't need XRM on the host if you're using Docker but you do otherwise.
You can use ``xbutil validate --device <device id>`` to confirm that your FPGA is ready to use.
If this executable is not on your PATH, it should be in ``/opt/xilinx/xrt/bin``.

You will also need the XCLBINs corresponding to your card on the host.
By default, these will be placed in ``/opt/xilinx/overlaybins`` and will be mounted into the container if using Docker with the ``proteus`` script.
The ``XLNX_VART_FIRMWARE`` environment variable should point to the directory containing the XCLBINs needed for your FPGA.
