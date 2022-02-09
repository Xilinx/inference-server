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

.. _xmodel_example_python:

Running an XModel (Python)
==========================

This example walks you through the process to make an inference request to a custom XModel in Python.
If you haven't already, make sure to go through the :ref:`hello_world_python` example first!
We gloss over some of the details covered in the previous one.
The complete script used here is available: :file:`examples/python/custom_processing.py`.

User variables
--------------

Making an inference to an actual XModel that accepts an image requires some additional data from the user.
These variables are pulled out into a separate block to highlight them.

* Batch size: The DPU your XModel targets may have a preferred batch size and so we can use this value to create the optimally-sized request.
* XModel Path: The XModel you want to run should exist on a path where the server runs. Here, we use a ResNet50 model trained on the ImageNet dataset, which is an image classification model.
* Image Path: To test this model, we need to use an image. Here, we use a sample image included for testing. The image path is given relative to the Xilinx Inference Server repository.

.. literalinclude:: ../examples/python/custom_processing.py
    :start-after: +user variables:
    :end-before: for this image

Load a Worker
-------------

As in the previous example, we can start the server and set up our client object from :py:class:`RestClient`.
To make an inference request to a custom XModel, we use the **Xmodel** worker that we have to load first.
Some workers accept load-time parameters to configure different options.
The **Xmodel** worker is one such worker.
The parameter we add is to pass the path to the XModel that we want to use.

.. literalinclude:: ../examples/python/custom_processing.py
    :start-after: +load worker:
    :end-before: -load worker:
    :dedent: 4

Get Images
----------

Now, we can prepare our request.
In this example, we use one image and duplicate it *batch_size* times so we can create one whole batch.

.. literalinclude:: ../examples/python/custom_processing.py
    :start-after: +get images:
    :end-before: -get images:
    :dedent: 4

Inference
---------

Using our images, we can construct a request to Xilinx Inference Server.
The ``ImageInferenceRequest`` class is a helper class that simplifies creating a request in the right format.
It accepts an image or a list of images and an optional boolean parameter that indicates whether the request should store the images directly as a tensor of RGB values.
By default, images are saved as base64-encoded strings however the Xmodel worker requires that the data is a tensor so we add *True*.

.. literalinclude:: ../examples/python/custom_processing.py
    :start-after: Construct the request
    :end-before: Can optionally post-process
    :dedent: 4

The data we receive from the response is a list of numbers.

Adding pre- and post-processing
-------------------------------

Depending on what model is run, you may need to add pre- and post-processing to the request for a useful inference.
For this model, we do need to apply pre- and post-processing to get accurate classifications.
To double-check our inference, we can check it against what we expect to receive.
For this test image and XModel, the top-5 classifications expected are listed.
259 is the most probable and this index corresponds to *Pomeranian* in the ImageNet labels.

.. literalinclude:: ../examples/python/custom_processing.py
    :start-after: for this image
    :end-before: -user variables
    :dedent: 4

For pre-processing, you can add custom logic to perform the necessary actions to the images prior to constructing the request.
Similarly, post-processing can be added after the data is received.
The pre- and post-processing functions used in this example can be seen in the source file.

.. literalinclude:: ../examples/python/custom_processing.py
    :start-after: +inference:
    :end-before: -inference:
    :dedent: 4
