..
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

MIGraphX
========

Using the AMD Inference Server with MIGraphX and GPUs requires some additional setup prior to use.

Set up the host and GPUs
------------------------



Build an image
--------------

To build an image with MIGraphX enabled, you need to add the ``--migraphx`` to the ``proteus dockerize`` command:

.. code-block:: bash

    # create the Dockerfile
    python3 docker/generate.py

    # build the dev image $(whoami)/proteus-dev-migraphx:latest
    ./proteus dockerize --migraphx --suffix="-migraphx"

    # build the production image $(whoami)/proteus-migraphx:latest
    ./proteus dockerize --migraphx --suffix="-migraphx" --production

Get assets and models
---------------------

You can download the assets and models used for tests and examples with:

.. code-block:: console

    $ ./proteus get --migraphx
