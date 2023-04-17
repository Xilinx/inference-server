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

Model Repository
================

A model repository is
.. include:: dry.rst
    :start-after: +define_model_repository
    :end-before: -define_model_repository

Single models
-------------

The directory structure for the model repository for a single model is:

.. code-block:: text

    /
    ├─ model_a/
    │  ├─ 1/
    │  │  ├─ <model>
    │  ├─ <config>
    ├─ model_b/
    |  ...

The model name, ``model_a`` in this template, must be unique among the models loaded on a particular server.
This name is used to name the endpoint used to make inference requests to.
Under this directory, there must be a directory named ``1/`` containing the model file itself and a TOML file describing the configuration.
The model file can have an arbitrary name and the file extension depends on the type of the model.
This file, ``<config>`` in this template, can have any name though ``config.toml`` is suggested and will be used in this documentation.
You can also use ``.pbtxt`` format files for single models as well.
The configuration file contains metadata for the model.
Consider this example of an MNIST TensorFlow model:

.. code-block:: toml

    name = "mnist"
    platform = "tensorflow_graphdef"

    [[inputs]]
    name = "images_in"
    datatype = "FP32"
    shape = [28, 28, 1]

    [[outputs]]
    name = "flatten/Reshape"
    datatype = "FP32"
    shape = [10]

The name must match the name of the model directory, i.e. ``model_a``.
The platform identifies the type of the model and determines the file extension of the model file.
The supported platforms are:

.. csv-table::
    :header: Platform,Model file extension
    :widths: 90, 10
    :width: 22em

    ``tensorflow_graphdef``,``.pb``
    ``pytorch_torchscript``,``.pt``
    ``vitis_xmodel``,``.xmodel``
    ``onnx_onnxv1``,``.onnx``
    ``migraphx_mxr``,``.mxr``
    ``amdinfer_cpp``,``.so``

The inputs and outputs define the list of input and output tensors for the model.
The names of the tensors may be significant if the platform needs them to perform inference.

The equivalent configuration file as a ``.pbtxt`` file would be:

.. code-block:: text

    name: "mnist"
    platform: "tensorflow_graphdef"
    inputs [
        {
            name: "images_in"
            datatype: "FP32"
            shape: [28, 28, 1]
        }
    ]
    outputs [
        {
            name: "flatten/Reshape"
            datatype: "FP32"
            shape: [10]
        }
    ]

While the inference server will accept a configuration file in this format, note that TOML files take priority if both are present and this format does not support defining :ref:`ensembles <ensembles>`.

Ensembles
---------

With :term:`ensembles <Ensembles>`, the "model" actually consists of a set of models.
The directory structure for the model repository for ensembles is:

.. code-block:: text

    /
    ├─ model_a/
    │  ├─ 1/
    │  │  ├─ <model_0>
    │  │  ├─ <model_1>
    │  │  ├─ ...
    │  ├─ <config>
    ├─ model_b/
    |  ...


As a concrete example, consider a three stage ensemble:

1. ``cplusplus`` backend executing a ``base64_decode`` model: receive a base64-encoded JPEG image and decode it to an RGB array
2. ``cplusplus`` backend executing a ``invert_image`` model: invert every pixel in the input RGB image
3. ``cplusplus`` backend executing a ``base64_encode`` model: convert the input RGB image to JPEG, base64-encode it and send it back to client

This ensemble uses the ``cplusplus`` backend for each stage with different models.
The configuration file for this ensemble could be:

.. code-block:: toml
    :linenos:

    [[models]]
    name = "invert_image"
    platform = "amdinfer_cpp"
    id = "base64_decode.so"

    [[models.inputs]]
    name = "image_in"
    datatype = "STRING"
    shape = [1048576]
    id = ""

    [[models.outputs]]
    name = "image_out"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "preprocessed_image"

    [[models]]
    name = "execute"
    platform = "amdinfer_cpp"
    id = "invert_image.so"

    [[models.inputs]]
    name = "image_in"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "preprocessed_image"

    [[models.outputs]]
    name = "image_out"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "inverted_image"

    [[models]]
    name = "invert_image_postprocess"
    platform = "amdinfer_cpp"
    id = "base64_encode.so"

    [[models.inputs]]
    name = "image_in"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "inverted_image"

    [[models.outputs]]
    name = "image_out"
    datatype = "STRING"
    shape = [1048576]
    id = ""

This single configuration file lists multiple models reflecting the multiple model files that are in the repository.
Each model in the ensemble is marked with ``[[models]]`` and has a name and platform just like single models.
As before, the name is used to define the endpoint.
For :term:`chains <Chain>`, the first model's name should match the name of the parent directory because this is the endpoint that will be used to send requests to the whole ensemble.
Each model also has an ID field that should be the name of the model file corresponding to this stage of the ensemble because the ``1/`` directory will contain multiple model files.

You can define one or more input or output tensors for each model using multiple ``[[models.inputs]]`` or ``[[models.outputs]]`` tags, respectively.
As in the single model case, each input/output tensor has a name, type and shape with the same meaning.
In the ensemble case, they also have an ID.
For output tensors, the ID is a unique string labeling this tensor.
The ID for input tensors should match the ID of the output tensor that is feeding it.
Input tensors with an empty ID indicate that the data comes from the external client.
Similarly, output tensors with an empty output ID indicate that the data goes to the external client.

The model repository for this example using the above configuration file would be:

.. code-block:: text

    /
    ├─ invert_image/
    │  ├─ 1/
    │  │  ├─ base64_decode.so
    │  │  ├─ base64_encode.so
    │  │  ├─ invert_image.so
    │  ├─ config.toml
