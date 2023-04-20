..
    Copyright 2023 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Documentation
=============

This directory contains the raw files used to build the `online documentation <InferenceServerDocumentation>`_.
As such, this directory is not intended to be read directly because many of the links and images are only populated in the web documentation.

Build the docs
--------------

Building the documentation requires Sphinx, Doxygen, and many other packages.
The developer container has all of these dependencies and is currently the only supported way to build the documentation.
The regular CMake build of the inference server adds two targets, ``doxygen`` and ``sphinx``, that can be used to build the documentation.
In the development container:

.. code-block:: console

    $ amdinfer make doxygen
    $ amdinfer make sphinx

After running these commands, the documentation is built in ``build/docs/sphinx``.

Deploy the docs
---------------

The easiest way to deploy the documentation is to use the ``deploy.sh`` script.
This script must run in the development container and you must first build the inference server with ``amdinfer build`` beforehand.
If you want to deploy to a specific version of the documentation, update the ``VERSION`` variable in the script prior to executing it.
This script will update the links in the README to point to the appropriate version, build the documentation as described above, and push the built documentation to the ``gh-pages`` branch.
