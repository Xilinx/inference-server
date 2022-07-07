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

Running a Vitis AI XModel (C++)
===============================

This example walks you through the process to make an inference request to a custom XModel in C++ using two methods: the native C++ API and the gRPC API.
This example is similar to the :ref:`xmodel_example_python` one but it uses C++ to create a new executable instead of making requests to a server if using the native C++ API.
With the gRPC API, our executable instead makes a gRPC request to the inference server.
We first show the example with the native C++ API and then highlight the differences if you were using the gRPC API instead.
The complete program used here is available: :file:`examples/cpp/custom_processing.cpp`.

Include the library
-------------------

AMD Inference Server's C++ API allows you to write your own C++ applications that link against AMD Inference Server's backend.
This approach bypasses the overheads associated with serialization that occurs with REST-based Python inferencing.
The public API is defined in :file:`proteus/proteus.hpp` and we include it here.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +include:
    :end-before: -include:
    :language: cpp

User variables
--------------

Making an inference to an actual XModel that accepts an image requires some additional data from the user.
These variables are pulled out into a separate block to highlight them.

* Batch size: The DPU your XModel targets may have a preferred batch size and so we can use this value to create the optimally-sized request.
* XModel Path: The XModel you want to run should exist on a path where the server runs. Here, we use a ResNet50 model trained on the ImageNet dataset, which is an image classification model.
* Image Path: To test this model, we need to use an image. Here, we use a sample image included for testing.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +user variables:
    :end-before: for this image
    :language: cpp
    :dedent: 2

Initialize
----------

Before calling any of other methods in the API, we need to first initialize AMD Inference Server.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +initialize:
    :end-before: -initialize:
    :language: cpp
    :dedent: 2

Load a worker
-------------

After initialization, we have to load the worker(s) we need for inference.
To make an inference request to a custom XModel, we use the **Xmodel** worker.
Some workers accept load-time parameters to configure different options.
The **Xmodel** worker is one such worker.
The parameter we add is to pass the path to the XModel that we want to use.
If the worker we're using doesn't accept or need load-time parameters, a null pointer can be passed instead.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +load native:
    :end-before: -load native:
    :language: cpp
    :dedent: 2

The return value from the load is the qualified name that should be used for subsequent operations such as inference.

Get images
----------

Now, we can prepare our request.
In this example, we use one image and duplicate it *batch_size* times.
The ResNet50 model we're using requires some pre-processing before running inference so we add a pre-processing step, which will open the images, resize them to the appropriate size for the model, normalize their values and return the images as vectors of data.
The implementation of the pre-processing function can be seen in the example's source.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +prepare images:
    :end-before: -prepare images:
    :language: cpp
    :dedent: 2

Construct request
-----------------

Using our images, we can construct a request to AMD Inference Server's backend.
We hardcode the shape of the image to the size we know that the pre-processing enforces.
We also pass the data-type of the data, which we again know to be an signed 8-bit integer from the pre-processing.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +construct request:
    :end-before: -construct request:
    :language: cpp
    :dedent: 2

Inference
---------

We can then send the request to AMD Inference Server by passing in a name from the output of a previous ``load()`` and the request itself.
With the native C++ API, enqueueing the request returns a Future object that we can later check to get the response.
For now, since there's only one, we can call ``get()`` on a Future object, which will block until the response is available.
The Future object will return a :cpp:class:`proteus::InferenceResponse` object.
This object can be parsed to analyze the data.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +inference native:
    :end-before: -inference native:
    :language: cpp
    :dedent: 2

Check the response
------------------

For this model, we need to post-process the raw output to make a useful classification for our image.
For each output, we post-process the results to extract the top *k* indices for the classification.
We can check this against our expected golden output to confirm that the inference is correct.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +validate:
    :end-before: -validate:
    :language: cpp
    :dedent: 2

Using gRPC
----------

Using the gRPC API instead just requires a few changes in how requests are made to the server.

Initialize
^^^^^^^^^^

In addition to the regular initialization, using the gRPC API requires creating a gRPC client, which defines how we can interact with the gRPC server.
We construct it with the address of the gRPC server.
Here, we're also starting the gRPC server directly by this application at a known port.
Of course, if the gRPC server already exists, then we don't need to start it in the client application.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +initialize grpc:
    :end-before: -initialize grpc:
    :language: cpp
    :dedent: 2

Load a worker
^^^^^^^^^^^^^

As before, we need to load the appropriate worker.
The only difference is that we use our gRPC client object to make the request.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +load grpc:
    :end-before: -load grpc:
    :language: cpp
    :dedent: 2

Inference
^^^^^^^^^

After preparing the request, we can make a synchronous request to the gRPC server using the gRPC API using the worker name returned from the ``load``.

.. literalinclude:: ../examples/cpp/custom_processing.cpp
    :start-after: +inference grpc:
    :end-before: -inference grpc:
    :language: cpp
    :dedent: 2
