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

The Vitis AI XModel backend executes an XModel on an AMD FPGA.
It uses :term:`VART` to run the XModel.

Model support
-------------

The Vitis AI backend should support most XModels that have one DPU subgraph and also supports models with multiple input and output tensors.
The tested models are listed below:

.. csv-table::
    :header: Model type,Tested models

    Classification,ResNet50
    Object detection,DenseBox; YOLOv3

Other models should also work but are currently untested.

Hardware support
----------------

While not every model is tested on every FPGA, the Vitis AI backend has run at least one model on the following devices:

.. csv-table::
    :header: Family,FPGA,DPU

    Alveo,U250,DPUCADF8H
    Versal,VCK5000,DPUCVDX8H
    Alveo,V70,DPUCV2DX8G

Other devices and DPUs may also work but are currently untested.

Host setup
----------

The details for setting up your host are available online in the :vitisAItree:`Vitis AI <board_setup>` repository.
These instructions will depend on the FPGAs you are targeting.
In essence, you will need to install software, program the shell on the FPGA(s) and download its ``xclbin`` files.

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

The ``xclbin`` files define the DPU that will run on the FPGA.
By default, these are installed in ``/opt/xilinx/overlaybins/<DPU>/*``
The environment variable ``XLNX_VART_FIRMWARE`` must be set in the container where the inference server is running to point to the directory where the ``xclbin`` files for your FPGA are.
You can mount this directory in your container to enable it to access these files.
Alternatively, you can also copy ``xclbin`` files directly into the container as well.

Build an image
--------------

To build an image with the Vitis AI backend enabled, you need to add the ``--vitis`` to the ``amdinfer dockerize`` command:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the development image $(whoami)/amdinfer-dev:latest
    ./amdinfer dockerize --vitis

    # build the development image $(whoami)/amdinfer-dev-vitis:latest
    ./amdinfer dockerize --vitis --suffix="-vitis"

    # build the deployment image $(whoami)/amdinfer-vitis:latest
    ./amdinfer dockerize --vitis --suffix="-vitis" --production

Start a container
-----------------

Depending on your use case and how you are using the server, you can start a container to use this backend in multiple ways.

Deployment
^^^^^^^^^^

You can start a deployment container with something like:

.. code-block:: console

    $ docker run --device /dev/xclmgmt<id> --device /dev/dri [--volume ...]

These ``--device`` flags pass the FPGA(s) to the container and you can mount other directories as needed to make models available.
The name of the ``xclmgmt`` file depends on your host so look in the ``/dev/`` directory to see what the file is.
If you have multiple FPGAs, there will be multiple such files.
The deployment image will need ``xclbin`` files.
There are a few ways to achieve this:

1. Copy ``xclbin`` files into ``./external/overlaybins/*`` before invoking the ``dockerize`` command to build the development image. The Dockerfile is configured to copy any files and directories from this path into the deployment image under ``/opt/xilinx/overlaybins``.
2. Copy ``xclbin`` files into the image after building it
3. Mount the ``xclbin`` files in the container at start time

For deploying on Kubernetes-based platforms, you will need to add the FPGA as a resource to your deployment after installing the `Xilinx FPGA Kubernetes plugin <https://github.com/Xilinx/FPGA_as_a_Service/tree/master/k8s-device-plugin>`__.
The exact name of the resource will depend on which FPGA and shell you are trying to request.

.. code-block:: yaml

    # rest of the deployment logic
    resources:
      limits:
        cpu: "1"
        memory: 2Gi
        xilinx.com/fpga-xilinx_u250_gen3x16_xdma_shell_3_1-0: 1

Development
^^^^^^^^^^^

A development container can be started with:

.. code-block:: console

    $ ./amdinfer run --dev

This automatically adds the detected devices, publishes ports, and mounts some convenient directories, such as your SSH directory, and drops you into a terminal in the container.
In particular, it will mount the default ``xclbin`` files directory from your host to the container.

Get test assets
---------------

You can download the assets and models used with this backend for tests and examples with:

.. code-block:: console

    $ ./amdinfer get --vitis --all-models

Loading the backend
-------------------

.. include:: /dry.rst
    :start-after: +loading_the_backend_intro
    :end-before: -loading_the_backend_intro

.. tabs::

    .. code-tab:: c++ C++

        // amdinfer::Client* client;
        // amdinfer::ParameterMap parameters;
        std::string endpoint = client->workerLoad("xmodel", parameters)

    .. code-tab:: python Python

        # client = amdinfer.Client()
        # parameters = amdinfer.ParameterMap()
        endpoint = client.workerLoad("xmodel", parameters)

.. include:: /dry.rst
    :start-after: +loading_the_backend_modelLoad
    :end-before: -loading_the_backend_modelLoad

Parameters
^^^^^^^^^^

You can provide the following backend-specific parameters at load-time:

.. csv-table::
    :header: Parameter,Type,Usage

    ``model``,string,Full path to the XModel to load
    ``threads``,integer,Number of threads to use in the thread pool for the backend. Defaults to 3.

Troubleshooting
---------------

If you run into problems, first check the :ref:`general troubleshooting guide <troubleshooting:Troubleshooting>` guide.
Then continue on to this XModel specific troubleshooting guide.
You will need access to the machine where the inference server is running to debug.

First things to check
^^^^^^^^^^^^^^^^^^^^^

Using FPGAs may not work as expected for a variety of reasons ranging from loose cables to misconfiguration in the container.
Always do these first when debugging any issue to eliminate the most common sources of error.

1. Use ``xbutil examine`` and ``xbutil validate`` on the host machine to first make sure your host machine can use the attached FPGA. If it doesn't work, check the logs. XRT will log error messages to the system log. Use ``journalctl`` or ``dmesg`` to view them.
2. Make sure the XRT version matches in the host and in the container with ``xbutil --version``
3. Optionally, repeat the ``xbutil examine`` and ``xbutil validate`` tests from the container. If this doesn't work, double-check how you are passing device files to the container.
4. Make sure the XRM daemon is running in the container with ``sudo systemctl status xrmd``
5. Run your failing test in the container and see if there are logs in the host machine's system logs.

Gotchas
^^^^^^^

Some common easy-to-make errors are listed here:

* Version mismatch: the XRT version on your host machine and the container must match
* ``xclbin`` mismatch: the same ``xclbin`` files must exist at the same location in the host machine and the container. The easiest way to guarantee this is to mount the ``xclbin`` directory into the container. If you copy files in to the container, make sure they're in the same locations as on the host.

CU timeouts
^^^^^^^^^^^

The CU refers to a compute unit on the FPGA and a timeout means it did not respond in time.
This is usually a misconfiguration of the FPGA.
The system logs on the host should help identify the problem.
If not, you also try resetting the FPGA with ``xbutil reset`` or flashing the shell on the FPGA again with ``xbmgmt``.

.. |platform| replace:: ``vitis_xmodel``
