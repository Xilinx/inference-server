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

Running ResNet50 - Python
=========================

This page walks you through the Python versions of the ResNet50 examples.
These examples and script are intended to run in the dev container.
You can see the full source files in the `repository <https://github.com/Xilinx/inference-server/tree/main/examples/resnet50>`__ for more details.

The inference server binds its C++ API to Python so the Python usage and functions look similar to their C++ counterparts but there are some differences due to the available features in both languages.
You should read the C++ version of this documentation to better compare these two.

In the dev container, the Python library is automatically built and installed as part of the CMake build process.
You may also install the Python library by installing a wheel.

.. note::
    These examples are intended to demonstrate the API and how to communicate with the server.
    They are not intended to show the most optimal performance for each backend.

Include the module
------------------

You can include the Python library by importing the ``amdinfer`` module.
The different submodules are imported by default but you can include them manually as well.
These explicit imports can help IDEs resolve Python imports for autocompletion.

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +import
    :end-before: -import
    :language: py

Start the server
----------------

This example assumes that the server is not running elsewhere and starts it locally from Python instead.
Creating the ``amdinfer.Server`` object starts the inference server backend and it stays alive as long as the object remains in scope.

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +initialize
    :end-before: -initialize
    :language: py
    :dedent: 8

Depending on what protocol you want to use to communicate with the server, you may need to start it explicitly using one of the object's methods.

For example, if you were using HTTP/REST:

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +start protocol
    :end-before: -start protocol
    :language: py
    :dedent: 8

If the server is already running somewhere, you don't need to do this.

Create the client object
------------------------

As in C++, you can create a client in Python corresponding to the protocol you want to use to talk to the server.

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +create client
    :end-before: -create client
    :language: py
    :dedent: 4


Load a worker
-------------

Once the server is ready, you have to load a worker to handle your request.
This worker, and any load-time parameters it accepts, are backend-specific.
After loading the worker, you get back an endpoint string that you use to make requests to this worker.
If a worker is already ready on the server or you already have an endpoint, then you don't need to do this.

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +load
    :end-before: -load
    :language: py
    :dedent: 4

After loading a worker, make sure it's ready before attempting to make an inference.

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +wait model ready
    :end-before: -wait model ready
    :language: py
    :dedent: 4

Prepare images
--------------

Depending on the model, you may need to perform some preprocessing of the data before making an inference request.
For ResNet50, this preprocessing generally consists of resizing the image, normalizing its values, and possibly converting types but its exact implementation depends on the model and on what the worker expects.
The implementations of the preprocessing functions can be seen in the examples' sources and they differ for each backend.

In these examples, you can pass a path to an image or to a directory to the executable.
This single path gets converted to a List of paths containing just the path you passed in or paths to all the files in the directory you passed in and its passed to the ``preprocess`` function.
The file at each path is opened as a ``numpy`` array and stored in a List that the preprocess function returns.

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +prepare images
    :end-before: -prepare images
    :language: py
    :dedent: 4

Construct requests
------------------

Using the images after preprocessing, you can construct requests to the inference server.
Each image is constructed into an ``InferenceRequest`` using the ``ImageInferenceRequest`` helper function.
This function accepts a single ``numpy`` array or a list of ``numpy`` arrays, where each array represents an input tensor.
The ResNet50 model only accepts a single input tensor so a single image is enough.
This function infers the shape and datatype of the image using the properties stored in the ``numpy`` array.

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +construct request
    :end-before: -construct request
    :language: py

Make an inference
-----------------

There are multiple ways of making a request to the inference server, some of which are used in the different implementations of these examples.
Before processing the response, you should verify it's not an error.

Then, you can examine the outputs and, depending on the model, postprocess the results.
For ResNet50, the raw results from the inference server lists the probabilities for each output class.
The postprocessing identifies the highest probability classes and the top few of these are printed using the labels file to map the indices to a human-readable name.

Here are some examples of making an inference used in these examples:

This is the simplest way to make an inference: a blocking single inference where you loop through all the requests and make requests to the server one at a time.

.. literalinclude:: ../examples/resnet50/vitis.py
    :start-after: +run inference
    :end-before: -run inference
    :language: py
    :dedent: 4

There are also some helper methods that wrap the basic inference APIs provided by the client.
The ``inferAsyncOrdered`` method accepts a list of requests, makes all the requests asynchronously using the C++ ``modelInferAsync`` API, waits until each request completes, and then returns a list of responses.
If there are multiple requests sent in this way, they may be batched together by the server.

.. literalinclude:: ../examples/resnet50/migraphx.py
    :start-after: +run inference
    :end-before: -run inference
    :language: py
    :dedent: 4
