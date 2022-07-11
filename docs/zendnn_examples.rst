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

.. _zendnn_examples:

ZenDNN Examples
===============

There are two examples provided in the repo (Python API and C++ API) for both TensorFlow and PyTorch.

Python API
----------

Python examples below will do the following:

1. Start the AMD Inference Server on HTTP port 8998
2. Load the AMD Inference Server with the specified model file
3. Read the image specified / Create dummy data
4. Sends the data to the AMD Inference Server over HTTP
5. Get the result back from the AMD Inference Server over HTTP
6. Post process if any and display the output

TensorFlow + ZenDNN
^^^^^^^^^^^^^^^^^^^

The python example is available at :code:`examples/python/tf_zendnn.py`.

1. To run the example with a real image:

    .. code-block:: console

        $ python examples/python/tf_zendnn.py --graph ./external/tensorflow_models/resnet_v1_50_baseline_6.96B_922.pb --image_location ./tests/assets/dog-3619020_640.jpg

2. To run the example with dummy data:

    .. code-block:: console

        $ python examples/python/tf_zendnn.py --graph ./external/tensorflow_models/resnet_v1_50_baseline_6.96B_922.pb --batch_size 16 --steps 4

    The above command will run the example with dummy data (4 requests with 16 dummy images each). This can be used as a functional test.

For more options, check the help with:

.. code-block:: console

    $ python examples/python/tf_zendnn.py --help


PyTorch + ZenDNN
^^^^^^^^^^^^^^^^

The python example is available at :code:`examples/python/pt_zendnn.py`.

1. To run the example with a real image:

    .. code-block:: console

        $ python examples/python/pt_zendnn.py --graph ./external/pytorch_models/resnet50_pretrained.pt --image_location ./tests/assets/dog-3619020_640.jpg

2. To run the example with dummy data:

    .. code-block:: console

        $ python examples/python/pt_zendnn.py --graph ./external/pytorch_models/resnet50_pretrained.pt --batch_size 16 --steps 4

    The above command will run the example with dummy data (4 requests with 16 dummy images each). This can be used as a functional test.

For more options, check the help with:

    .. code-block:: console

        $ python examples/python/pt_zendnn.py --help

C++ API
-------

The C++ API bypasses the HTTP server and connects directly to the Inference Server.
The flow is as follows:

1. Load the AMD Inference Server with the specified model file
2. Read the image specified / Create dummy data and prepare input
3. The data is packed into an Interface object and pushed to a queue
4. Retrieve the result back from the AMD Inference Server
5. Post process if any and display the output

The C++ example will be built when the server is being built according to the packages available.

* To run the C++ example with real image, provide :code:`image_location` in :code:`Option` struct in source code.
* If :code:`image_location` is set to :code:`""` in :code:`Option`, dummy data will be used. This can be used for benchmarking.

TensorFlow + ZenDNN
^^^^^^^^^^^^^^^^^^^

Source is available at :code:`examples/cpp/tf_zendnn_client.cpp`. To build and run the example:

.. code-block:: console

    $ ./proteus build --debug && ./build/Debug/examples/cpp/tf_zendnn_client

PyTorch + ZenDNN
^^^^^^^^^^^^^^^^

Source is available at :code:`examples/cpp/pt_zendnn_client.cpp`. To build and run the example:

.. code-block:: console

    $ ./proteus build --debug && ./build/Debug/examples/cpp/pt_zendnn_client
