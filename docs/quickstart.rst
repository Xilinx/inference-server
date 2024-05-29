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

Quickstart
==========

This quickstart shows you how to deploy the server locally and send inference requests to it.
These steps elaborate on the quick start example described in the project :github:`README <Xilinx/inference-server#quick-start-deployment-and-inference>`.
The README example uses a script that takes care of the setup that is manually performed here but this guide should help you better understand what is needed to deploy the server and use it.
If you already have the server deployed somewhere, you can skip ahead to the :ref:`inference section <deploymentSummary>`.

.. note::

    This guide shows you how to get started quickly.
    It is not intended to demonstrate all the different options and settings that could be applied at different stages.

Prerequisites
-------------

For making an inference:

* A Linux host machine with Python 3.6+

For deploying the server:

* A Linux host machine with `Docker <LinkDockerInstallLinux_>`_ installed
* Sufficient disk space to host your models

This guide assumes that inference will be made from the same machine where the server is deployed.

Prepare the model repository
----------------------------

A model repository is |define_model_repository|.

.. code-block:: text

    /
    ├─ resnet50/
    │  ├─ 1/
    │  │  ├─ <model>
    │  ├─ config.toml

The model name, ``resnet50`` in this template, must be unique among the models loaded on a particular server and it must match the name used in the configuration TOML file.
This name is used to name the endpoint that clients use to make inference requests.
Under this directory, there must be a directory named ``1/`` containing the model file itself and a TOML file containing the metadata for the model.
This TOML file may be named anything but ``config.toml`` is suggested and used throughout this documentation.
The model file can also have an arbitrary name and the file extension depends on the type of the model.

In this guide, you will deploy a single ResNet50 model.
You can create a model repository in your current working directory using these steps.
Depending on what hardware you want to use and have on your host machine, you can use the appropriate model.
The CPU version has no special hardware requirements to run so you can always run that.

.. tabs::

    .. code-tab:: console CPU

        $ wget -O tensorflow.zip https://www.xilinx.com/bin/public/openDownload?filename=tf_resnetv1_50_imagenet_224_224_6.97G_2.5.zip
        $ unzip -j "tensorflow.zip" "tf_resnetv1_50_imagenet_224_224_6.97G_2.5/float/resnet_v1_50_baseline_6.96B_922.pb" -d .
        $ mkdir -p ./model_repository/resnet50/1
        $ mv ./resnet_v1_50_baseline_6.96B_922.pb ./model_repository/resnet50/1/

    .. code-tab:: console GPU

        $ wget https://github.com/onnx/models/raw/5faef4c33eba0395177850e1e31c4a6a9e634c82/vision/classification/resnet/model/resnet50-v2-7.onnx
        $ mkdir -p ./model_repository/resnet50/1
        $ mv ./resnet50-v2-7.onnx ./model_repository/resnet50/1/

    .. code-tab:: console FPGA

        # this example assumes a U250. If you're using a different board, download the appropriate model for your board instead
        $ wget -O vitis.tar.gz https://www.xilinx.com/bin/public/openDownload?filename=resnet_v1_50_tf-u200-u250-r2.5.0.tar.gz
        $ tar -xzf vitis.tar.gz "resnet_v1_50_tf/resnet_v1_50_tf.xmodel"
        $ mkdir -p ./model_repository/resnet50/1
        $ mv ./resnet_v1_50_tf/resnet_v1_50_tf.xmodel ./model_repository/resnet50/1/

For the models used here, you can save their corresponding ``config.toml`` to the correct path with:

.. tabs::

    .. code-tab:: shell CPU

        cat <<EOF > "./model_repository/resnet50/config.toml"
        name = "resnet50"
        platform = "tensorflow_graphdef"

        [[inputs]]
        name = "input"
        datatype = "FP32"
        shape = [224, 224, 3]

        [[outputs]]
        name = "resnet_v1_50/predictions/Reshape_1"
        datatype = "FP32"
        shape = [1000]
        EOF

    .. code-tab:: shell GPU

        cat <<EOF > "./model_repository/resnet50/config.toml"
        name = "resnet50"
        platform = "onnx_onnxv1"

        [[inputs]]
        name = "input"
        datatype = "FP32"
        shape = [224, 224, 3]

        [[outputs]]
        name = "output"
        datatype = "FP32"
        shape = [1000]
        EOF

    .. code-tab:: shell FPGA

        cat <<EOF > "./model_repository/resnet50/config.toml"
        name = "resnet50"
        platform = "vitis_xmodel"

        [[inputs]]
        name = "input"
        datatype = "INT8"
        shape = [224, 224, 3]

        [[outputs]]
        name = "output"
        datatype = "INT8"
        shape = [1000]
        EOF

The name must match the name of the model directory: it defines the endpoint that will be used for inference.
The platform identifies the type of the model and determines the file extension of the model file.

The inputs and outputs define the list of input and output tensors for the model.
The names of the tensors may be significant if the backend needs them to perform inference.

Get the deployment image
------------------------

The deployment image is optimized for size and only contains the run-time dependencies of the server to allow for quicker deployments.
It has limited debugging capabilities and it contains a precompiled executable for the server that automatically starts when the container starts.
You can pull the :ref:`deployment image <deployment:Deployment image>` if it exists or build it yourself.

.. include:: dry.rst
    :start-after: +docker_pull_deployment_images
    :end-before: -docker_pull_deployment_images

Start the image
---------------

You can start a container from the deployment image with ``docker run`` as any other image.
For more information about these flags and other options you can use, refer to its `documentation <LinkDockerRun_>`_.
The flags used in this sample command are:

* ``--volume $(pwd)/model_repository:/mnt/models:rw``: mount the model repository you created above into the container to where the server expects it to be. This allows the server to load these models at startup.
* ``--net=host``: use the host networking in the container. By default, the server uses ports 8998 and 50051 for HTTP and gRPC communication, respectively. By using this flag, you can use these same ports on the host to send requests to the server in the container.
* ``--device <path/to/device/file>``: pass the specified files into the server. If you are using hardware accelerators, these devices need to be passed into the container so they can be used. For GPUs, this will be ``--device /dev/kfd --device /dev/dri``. For FPGAs, this will be ``--device /dev/dri --device /dev/xclmgmt<id>``, where the ID of the ``xclmgmt`` device will depend on your particular machine.

.. tip::

    Using ``--net=host`` will not work if the default ports are unavailable on your host machine.
    In this case, you can omit this flag and use ``--publish`` to map these ports in the container to other ports on your host machine.
    Refer to the ``docker run`` documentation for more information.

.. tabs::

    .. code-tab:: console CPU

        $ docker run -d --volume $(pwd)/model_repository:/mnt/models:rw --net=host amdih/serve:uif1.2_zendnn_amdinfer_0.4.0

    .. code-tab:: console GPU

        $ docker run -d --device /dev/kfd --device /dev/dri --volume $(pwd)/model_repository:/mnt/models:rw --net=host amdih/serve:uif1.2_migraphx_amdinfer_0.4.0

    .. code-tab:: console FPGA

        $ docker run -d --device /dev/dri --device /dev/xclmgmt<id> --volume $(pwd)/model_repository:/mnt/models:rw --net=host amdih/serve:uif1.2_vai_amdinfer_0.4.0

The endpoints for each model will be the name of the model in the ``config.toml``, which should match the name of the parent directory in the model repository.
In this example, it would be "resnet50".
You are now ready to make requests to this server.

.. _deploymentSummary:

Server deployment summary
-------------------------

After setting up the server as above, you have the following information:

* IP address: 127.0.0.1 since the server is running on the same machine where you will run the inference
* Ports: 8998 and 50051 for HTTP and gRPC, respectively. If you used ``--publish`` in the ``docker run`` command to remap the ports, your port numbers may be different and you can see what they are using ``docker ps``.
* Endpoint: "resnet50" since that is what the model name was used in the model repository and in the configuration file

The rest of this example will use these values in the sample code so substitute your own values if they are different.

Get the Python library
----------------------

The ``amdinfer`` client library allows you to make clients that you can use to communicate with the server over any protocol that the server supports.
Clients for different protocols have the same base set of methods so you can easily replace one with another.
The library can be used from C++ or Python.
In this example, you will use the Python library, which you can install with ``pip``:

.. code-block:: console

    $ pip install amdinfer

Running an example
------------------

The `AMD Inference Server repository <LinkInferenceServerRepository_>`_ includes examples that demonstrate running an end-to-end inference request.
This particular example targets the ResNet50 model you've already deployed on the server above.
To perform the inference, you will need some files that are available in the repository:

.. parsed-literal::

    $ wget :amdinferRawFull:`examples/resnet50/imagenet_classes.txt`
    $ wget :amdinferRawFull:`tests/assets/dog-3619020_640.jpg`
    $ wget :amdinferRawFull:`examples/resnet50/resnet.py`

These files provide the class labels, a sample image and a helper Python script.
You will also need the particular example file corresponding to the same hardware your server is intended for.
You can download this file and run it to perform an inference on the server.
At a high-level, the example script performs pre-processing on the image, constructs a request and sends it to the server.
The server responds with the results of the inference.
These results are post-processed and the top 5 labels for the image are printed.

.. tabs::

    .. group-tab:: CPU

        .. parsed-literal::

            $ wget :amdinferRawFull:`examples/resnet50/tfzendnn.py`
            $ python3 tfzendnn.py --ip 127.0.0.1 --grpc-port 50051 --endpoint resnet50 --image ./dog-3619020_640.jpg --labels ./imagenet_classes.txt --wait

    .. group-tab:: GPU

        .. parsed-literal::

            $ wget :amdinferRawFull:`examples/resnet50/migraphx.py`
            # This will take some time initially as MIGraphX will compile the ONNX model to MXR
            $ python3 migraphx.py --ip 127.0.0.1 --http-port 8998 --endpoint resnet50 --image ./dog-3619020_640.jpg --labels ./imagenet_classes.txt --wait

    .. group-tab:: FPGA

        .. parsed-literal::

            $ wget :amdinferRawFull:`examples/resnet50/vitis.py`
            $ python3 vitis.py --ip 127.0.0.1 --http-port 8998 --endpoint resnet50 --image ./dog-3619020_640.jpg --labels ./imagenet_classes.txt --wait

After running the script, you should get output similar to the following.
The exact output may be slightly different depending on whether you used CPU, GPU or FPGA versions of the example.

.. code-block:: text

    Running the TF+ZenDNN example for ResNet50 in Python
    Waiting until the server is ready...
    Making inferences...
    Top 5 classes for ./dog-3619020_640.jpg:
      n02112018 Pomeranian
      n02112350 keeshond
      n02086079 Pekinese, Pekingese, Peke
      n02112137 chow, chow chow
      n02113023 Pembroke, Pembroke Welsh corgi

Inference summary
-----------------

To perform inference, you need to write an application that uses the AMD Inference Server's client library to construct a request and send it to the server.
Communicating with the server requires you to know its address, IP and port, as well as an endpoint.
The endpoint uniquely identifies a particular model running in the server.
When there are multiple models running, the endpoint becomes important to differentiate where to send to send the request.
The server responds to the request and you can examine the results to take further action.

Next steps
----------

To learn more about the Python script you ran to run the inference, walk through the script in this :ref:`example <example_resnet50_python:Running ResNet50 - Python>`.
