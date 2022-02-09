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

.. _hello_world_python:

Hello World (Python)
====================

Xilinx Inference Server's Python API allows you to start the server and send requests to it from Python.
This example walks you through how to start the server and use Python to send requests to a simple model.
The complete script used here is available: :file:`examples/python/hello_world_rest.py`.

Import the library
------------------

We need to bring in the Xilinx Inference Server Python library.
The library's source code is in :file:`src/python` and it gets installed in the dev container on startup.

.. literalinclude:: ../examples/python/hello_world_rest.py
    :start-after: +imports:
    :end-before: -imports:

Create our client and server objects
------------------------------------

We assume that the server will be running locally with the default HTTP port and pass that to the client.
In this example, we'll be using REST to communicate to the server so we create a :py:class:`RestClient` object.

.. literalinclude:: ../examples/python/hello_world_rest.py
    :start-after: +create objects:
    :end-before: -create objects:
    :dedent: 4

Is Xilinx Inference Server already running?
-------------------------------------------

If the server is already started externally, we don't want to start it again.
So, we attempt to check if the server is live.
If this fails, we start the server ourselves.
Either way, the client will attempt to communicate to the server at ``http://localhost:8998``.

.. literalinclude:: ../examples/python/hello_world_rest.py
    :start-after: +start server:
    :end-before: -start server:
    :dedent: 4

Load a worker
-------------

Inference requests in Xilinx Inference Server are made to workers.
Workers are started as threads in Xilinx Inference Server and have a defined lifecycle.
Before making an inference request to a worker, it must first be loaded.
Loading a worker returns an identifier that the client should use for future operations.

This worker, Echo, is a simple example worker that accepts integers as input, adds one to the inputs and returns the sum.
We'll use it to demonstrate the Python flow.

.. literalinclude:: ../examples/python/hello_world_rest.py
    :start-after: +load worker:
    :end-before: -load worker:
    :dedent: 4

Inference
---------

Once the worker is ready, we can make an inference request to it.
We construct a request that contains five integers and send it to Xilinx Inference Server.
The ``NumericalInferenceRequest`` class is a helper class that simplifies creating a request in the right format.

.. literalinclude:: ../examples/python/hello_world_rest.py
    :start-after: +inference:
    :end-before: -inference:
    :dedent: 4

Validate the response
---------------------

Now, we want to check what our response is.
Here, we can simplify our checks because we already know what we expect to receive.
So we check is that the number of outputs match the number of inputs we used.
We also check that each output only has one index and is one more than the corresponding input since we know that's what the Echo worker does.

.. literalinclude:: ../examples/python/hello_world_rest.py
    :start-after: +validate:
    :end-before: -validate:
    :dedent: 4

Clean up
--------

Workers that are loaded in Xilinx Inference Server will persist until the server shuts down or they're explicitly unloaded.
While it's not shown here, the Python API provides an ``unload()`` method for this purpose.
Finally, if we started the server from Python, we shut it down before finishing.
If there are any loaded workers at this time, they will be cleaned up before shutdown.

.. literalinclude:: ../examples/python/hello_world_rest.py
    :start-after: +clean up:
    :end-before: -clean up:
    :dedent: 4
