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

.. _hello_world_python:

Hello World - Python
====================

AMD Inference Server's Python API allows you to start the server and send requests to it from Python.
This example walks you through how to start the server and use Python to send requests to a simple model.
This example and script are intended to run in the dev container.
The complete script used here is available in the repository.

Import the library
------------------

You need to bring in the AMD Inference Server Python library to use the API.
The Python library is based on the Inference Server's C++ API and it gets installed in the dev container when you build the project.
You can also install it using a pre-built wheel.

.. literalinclude:: ../examples/hello_world/echo.py
    :start-after: +imports
    :end-before: -imports

Create our client and server objects
------------------------------------

We assume that the server will be running locally with the default HTTP port and pass that to the client.
This example assumes that the server will be running locally with the default HTTP port.
Since you'll be using HTTP/REST to communicate with the server, you should create an `HttpClient` using the address of the server as the argument.


.. literalinclude:: ../examples/hello_world/echo.py
    :start-after: +create objects
    :end-before: -create objects
    :dedent: 4

Is AMD Inference Server already running?
----------------------------------------

If the server is already started externally, you don't want to start it again.
You can see if the server is live using the client object.
If it's not live, then you can start the server from Python by instantiating the ``Server`` object.
Depending on what protocol you're using to communicate with the server, that protocol may need to be started on the server.
The server will remain active as long as the server object stays in scope.

.. literalinclude:: ../examples/hello_world/echo.py
    :start-after: +start server
    :end-before: -start server
    :dedent: 4

Load a worker
-------------

Inference requests in AMD Inference Server are made to workers.
Workers are started as threads in AMD Inference Server and have a defined lifecycle.
Before making an inference request to a worker, it must first be loaded.
Loading a worker returns an endpoint that the client should use for future operations.

This worker, Echo, is a simple example worker that accepts an integer as input, adds one to it and returns the sum.

.. literalinclude:: ../examples/hello_world/echo.py
    :start-after: +load worker
    :end-before: -load worker
    :dedent: 4

Inference
---------

Once the worker is ready, you can make an inference request to it.
To do so, first construct a request.
We construct a request that contains an integer and send it to AMD Inference Server.

.. literalinclude:: ../examples/hello_world/echo.py
    :start-after: +inference
    :end-before: -inference
    :dedent: 4

To make a request, you need to create, at minimum, a ``proteus.InferenceRequest`` object and add ``proteus.InferenceRequestInput`` objects to it.
Each input object represents an input tensor for the request and has a number of metadata attributes associated with it such as a name, datatype, and shape.
This format is based on :github:`KServe's v2 specification <kserve/kserve/blob/master/docs/predict-api/v2/required_api.md>`.
For images, there's also a helper method called ``proteus.ImageInferenceRequest`` that you can use to create requests.
It's used in the ResNet50 Python examples.

.. literalinclude:: ../examples/hello_world/echo.py
    :start-after: +make request
    :end-before: -make request

Validate the response
---------------------

After making the inference, you should check what the response was.
First, make sure the inference didn't fail by checking if the response is erroneous.
Then, you can get the outputs and examine them.
The format and contents of the output data will depend on the worker and model used for inference.
In this case, the Echo worker returns a single output tensor back with one integer that should be one larger than what was sent.
The assertions here check these cases.

.. literalinclude:: ../examples/hello_world/echo.py
    :start-after: +validate
    :end-before: -validate
    :dedent: 4

Clean up
--------

Workers that are loaded in AMD Inference Server will persist until the server shuts down or they're explicitly unloaded.
While it's not shown here, the Python API provides an ``unload()`` method for this purpose.
As the script ends, the Server object's destructor will clean up any active workers on the server and it will shut down
