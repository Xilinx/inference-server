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

Running ResNet50 - C++
======================

This page walks you through the C++ versions of the ResNet50 examples.
These examples are intended to run in the development container because you need to build and compile these executables.
You can see the full source files used here in the :amdinferTree:`repository <examples/resnet50>` for more details.

The inference server binds its C++ API to Python so the Python usage and functions look similar to their C++ counterparts but there are some differences due to the available features in both languages.
You can read the :ref:`Python version of this example <example_resnet50_python:Running ResNet50 - Python>` to better compare these two.

.. note::
    These examples are intended to demonstrate the API and how to communicate with the server.
    They are not intended to show the most optimal performance for each backend.

Include the header
------------------

AMD Inference Server's C++ API allows you to write your own C++ client applications that can communicate with the inference server.
You can include the entire public API by including :file:`amdinfer/amdinfer.hpp` or selectively include header files as needed.

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +include:
    :end-before: -include:
    :language: cpp

Start the server
----------------

This example assumes that the server is not running elsewhere and starts it locally from C++ instead.
Creating the ``amdinfer::Server`` object starts the inference server backend and it stays alive as long as the object remains in scope.

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +initialize:
    :end-before: -initialize:
    :language: cpp
    :dedent: 2

Depending on what protocol you want to use to communicate with the server, you may need to start it explicitly using one of the ``amdinfer::Server`` object's methods.

For example, if you were using HTTP/REST:

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +start protocol:
    :end-before: -start protocol:
    :language: cpp
    :dedent: 2

With gRPC:

.. literalinclude:: ../examples/resnet50/tfzendnn.cpp
    :start-after: +start protocol
    :end-before: -start protocol
    :language: cpp
    :dedent: 2

The native C++ API does not need starting and just creating the Server object is sufficient.

If the server is already running somewhere, you don't need to do this.

Create the client object
------------------------

The ``amdinfer::Client`` base class defines how to communicate with the server over the supported protocols.
This client protocol is based on KServe's API.
Each protocol inherits from this class and implements its methods.
Some examples of clients that you can create:

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +create client:
    :end-before: -create client:
    :language: cpp
    :dedent: 2

.. literalinclude:: ../examples/resnet50/tfzendnn.cpp
    :start-after: +create client
    :end-before: -create client
    :language: cpp
    :dedent: 2

.. literalinclude:: ../examples/resnet50/ptzendnn.cpp
    :start-after: +create client
    :end-before: -create client
    :language: cpp
    :dedent: 2

Load a worker
-------------

Once the server is ready, you have to load a worker to handle your request.
This worker, and any load-time parameters it accepts, are backend-specific.
After loading the worker, you get back an endpoint string that you use to make requests to this worker.
If a worker is already ready on the server or you already have an endpoint, then you don't need to do this.

Here are some of the different workers you can start to perform inference on a ResNet50 model.

XModel - Vitis AI on AMD FPGA:

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +load:
    :end-before: -load:
    :language: cpp
    :dedent: 2

TF+ZenDNN - ZenDNN on AMD CPU:

.. literalinclude:: ../examples/resnet50/tfzendnn.cpp
    :start-after: +load
    :end-before: -load
    :language: cpp
    :dedent: 2

MIGraphX - MIGraphX on AMD GPU:

.. literalinclude:: ../examples/resnet50/migraphx.cpp
    :start-after: +load
    :end-before: -load
    :language: cpp
    :dedent: 2

After loading a worker, make sure it's ready before attempting to make an inference.

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +wait model ready:
    :end-before: -wait model ready:
    :language: cpp
    :dedent: 2

Prepare images
--------------

Depending on the model, you may need to perform some preprocessing of the data before making an inference request.
For ResNet50, this preprocessing generally consists of resizing the image, normalizing its values, and possibly converting types but its exact implementation depends on the model and on what the worker expects.
The implementations of the preprocessing functions can be seen in the examples' sources and they differ for each backend.

In these examples, you can pass a path to an image or to a directory to the executable.
This single path gets converted to a vector of paths containing just the path you passed in or paths to all the files in the directory you passed in and its passed to the ``preprocess`` function.
The file at each path is opened and stored in an ``std::vector<T>``, where ``T`` depends on the data type that a backend works with.
Since there may be many images, ``preprocess`` returns an ``std::vector<std::vector<T>>``.

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +prepare images:
    :end-before: -prepare images:
    :language: cpp
    :dedent: 2

Construct requests
------------------

Using the images after preprocessing, you can construct requests to the inference server.
For each image, you create an ``InferenceRequest`` and add input tensors to it.
The ResNet50 model only accepts a single input tensor so you just add one by specifying the image data, its shape, and data type.
In this example, you create a vector of such requests.

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +construct request:
    :end-before: -construct request:
    :language: cpp
    :dedent: 2

Make an inference
-----------------

There are multiple ways of making a request to the inference server, some of which are used in the different implementations of these examples.
Before processing the response, you should verify it's not an error.

Then, you can examine the outputs and, depending on the model, postprocess the results.
For ResNet50, the raw results from the inference server lists the probabilities for each output class.
The postprocessing identifies the highest probability classes and the top few of these are printed using the labels file to map the indices to a human-readable name.

Here are some examples of making an inference used in these examples:

This is the simplest way to make an inference: a blocking single inference where you loop through all the requests and make requests to the server one at a time.

.. literalinclude:: ../examples/resnet50/vitis.cpp
    :start-after: +validate:
    :end-before: -validate:
    :language: cpp
    :dedent: 2

You can also make a single asynchronous request to the server where you get back a ``std::future`` that you can use later to get the results of the inference.

.. literalinclude:: ../examples/resnet50/ptzendnn.cpp
    :start-after: +validate
    :end-before: -validate
    :language: cpp
    :dedent: 6

There are also some helper methods that wrap the basic inference APIs provided by the client.
The ``inferAsyncOrdered`` method accepts a vector of requests, makes all the requests asynchronously using the ``modelInferAsync`` API, waits until each request completes, and then returns a vector of responses.
If there are multiple requests sent in this way, they may be batched together by the server.

.. literalinclude:: ../examples/resnet50/migraphx.cpp
    :start-after: +validate:
    :end-before: -validate:
    :language: cpp
    :dedent: 2
