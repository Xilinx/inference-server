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

Quickstart - Development
========================

This quickstart is intended for a user who is developing, testing, debugging or otherwise working more deeply with the inference server.
There are multiple ways to build the server but this quickstart only covers Docker-based development using the included Python run script ``amdinfer``.

Prerequisites
-------------

* `Docker <https://docs.docker.com/get-docker/>`__
* `Docker-Compose <https://docs.docker.com/compose/install/>`__ (optional - used for some tests)
* Python3
* git
* Network access to clone the repository
* Sufficient disk space to download test data and assets


Set up the host
---------------

The inference server supports multiple platforms and hardware backends.
Ensure that you set up your host appropriately depending on which platform(s) you are using.
For example, to use GPUs, you will need to install the drivers.
More information on host setup can be found in :ref:`each platforms' guide <platforms:Platforms>`.

Get the code
------------

Use ``git`` to clone the repository from Github:

.. code-block:: console

    $ git clone https://github.com/Xilinx/inference-server.git
    $ cd inference-server

Tests and examples need assets like images and videos to run.
Some of these files are stored in `Git LFS <https://git-lfs.github.com/>`__.
Depending on your host, these files may be automatically downloaded with the ``git clone``.
If some of the files in ``tests/assets`` are very small (less than 300 bytes), then you haven't downloaded these Git LFS artifacts.
From your host or after entering the development container, use ``git lfs pull`` to get these files.

Build or get the Docker image
-----------------------------

You can pull the development image from a Docker registry if it's already built.

.. code-block:: console

    $ docker pull <image>

To build a development image from source, run the following commands:

.. tabs::

    .. code-tab:: console CPU

        $ python3 docker/generate.py
        $ ./amdinfer dockerize --ptzendnn /path/to/ptzendnn/archive --tfzendnn /path/to/tfzendnn/archive

    .. code-tab:: console GPU

        $ python3 docker/generate.py
        $ ./amdinfer dockerize --migraphx

    .. code-tab:: console FPGA

        $ python3 docker/generate.py
        $ ./amdinfer dockerize --vitis

The ``generate.py`` script is used to create a dockerfile in the root directory, which is then used by the ``dockerize`` command.
Then, this dockerfile is used to build the development image with the appropriate platform enabled.
You can also combine the platform flags to enable multiple platforms in one image.
Look at the :ref:`platform-specific documentation <platforms:Platforms>` for more information.
By default, this builds the development image as ``<username>/amdinfer-dev:latest``.

After the image is built, you can use it to start a container:

.. code-block:: console

    $ ./amdinfer run --dev

This command runs the default created development image created above.
The ``--dev`` preset will mount the working directory into ``/workspace/amdinfer/``, mount some additional directories into the container, expose some ports to the host and pass in device files for any available hardware.
Some options may be overridden on the command-line (use ``--help`` to see the options).
By default, it will open a Bash shell in this container and show you a splash screen to show that you've entered the container.

Compiling the AMD Inference Server
----------------------------------

These commands are all run inside the development container.
Here, :file:`./amdinfer` is aliased to :command:`amdinfer`.

.. code-block:: console

    $ amdinfer build

The build command builds the :program:`amdinfer-server` executable, the C++ examples and the C++ tests.
By default, this will be the debug version.
You can pass flags to ``build`` to control the compile options.

.. tip::

    When starting new containers or switching to different ones after having run build once, you may need to run ``amdinfer build --regen --clean`` initially.
    New containers mount the working directory and so stale artifacts from previous builds may be present.
    These two flags delete the CMake cache and do a clean build, respectively.

.. warning::

    In general, you should not use ``sudo`` to run ``amdinfer`` commands.
    Some commands create files in your working directory and using ``sudo`` creates files with mixed permissions in your container and host and will even fail in some cases.

The ``build`` will also install the server's Python library in the development container.

.. code-block:: console

    pip list | grep amdinfer

Get test artifacts
------------------

For running tests and examples, you need to get models and other files that they use.
Make sure you have `Git LFS <https://git-lfs.github.com/>`__ installed.
You can download all files, as shown below with the ``--all`` flag, or download platform-specific files by passing the appropriate flag(s).
Use ``--help`` to see the options available.

.. code-block:: console

    $ git lfs pull
    $ amdinfer get --all

You must abide by the license agreements of these files, if you choose to download them.

Run the AMD Inference Server
----------------------------

Once the server is built, you can run the test suite.
Make sure you have the relevant test artifacts as described in the previous section.

.. code-block:: bash

    amdinfer test

You can also run the examples by starting the server in one terminal in the container, opening another terminal in the same container and running an example script.

.. code-block:: bash

    # start the server
    amdinfer start

    # this command will block and the server will idle for requests
    # from a new terminal, you can send it requests

    # on the host, use this command to open a new terminal in the most recent
    # inference server container:
    ./amdinfer attach

    # run an example from the second terminal which will reuse the server from
    # the first terminal
    python ./examples/hello_world/echo.py

    # shutdown the server using Ctrl+C

Next steps
----------

Read more about the :ref:`architecture <architecture:Architecture>` of the server and dive into the code!
