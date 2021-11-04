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

Python Examples
===============

These examples demonstrate using Xilinx Inference Server with Python.

Custom Processing
-----------------

This script shows how to start the server from Python and make and validate an inference request using the Python REST API.
After starting the server and confirming that it's live, we perform some preprocessing on images before constructing a request.
An inference request is made to the XModel worker and telling it to run a Resnet50 classification model.
The response from Xilinx Inference Server is then postprocessed and checked against the expected output.

This example is derived from ``test_xmodel_0()`` in ``tests/python/test_xmodel.py``.
