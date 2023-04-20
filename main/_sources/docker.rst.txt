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

Docker
======

You can use the AMD Inference Server with Docker to deploy the server.
For more powerful and easier to use deployments, consider using :ref:`KServe <kserve:KServe>`.

Build the deployment Docker image
---------------------------------

The deployment image is optimized for size and only contains the runtime dependencies of the server to allow for quicker deployments.
It automatically starts the server executable as the image starts as opposed to opening a Bash shell though that can be overridden.
To build the deployment image:

.. code-block:: console

    $ python3 docker/generate.py
    $ ./amdinfer dockerize --production <platform flags>

Depending on what platforms you want to support, add the appropriate flags to enable :doc:`Vitis AI <vitis_ai:Vitis AI>`, :doc:`ZenDNN <zendnn:ZenDNN>` or :doc:`MIGraphX <migraphx:MIGraphX>`.
Refer to the help or the platform documentation for more information on how to build the right image.

.. important::

    Some platforms require additional steps before building the Docker image with them enabled.
    Look at the platform-specific documentation for more information about how to build images for a given platform.

.. _docker~push to a registry:

Push to a registry
^^^^^^^^^^^^^^^^^^

In some use cases, for example with Kubernetes or KServe, the image must be pushed to a Docker registry before it can be used.
If you don't have access to one, you can start a local registry using `these instructions <https://docs.docker.com/registry/deploying/>`__ from Docker.
Make sure to set up a secure registry if you need access to the registry from more than one host.

To push the image to the registry, re-tag the image with the registry and push it.
For example, if you're using the local registry approach from above, the registry name would be ``localhost:5000`` by default.
By default, using ``amdinfer dockerize ...`` builds an image of the form ``$(whoami)/<image>``, which is what the code snippet uses below.

.. code-block:: console

    $ docker tag $(whoami)/<image> <registry>/<image> && docker push <registry>/<image>

Once the image is pushed to the registry, verify that it can be pulled with Docker from all nodes in the Kubernetes cluster.
In some cases, Kubernetes may fail to pull the image, even if it's tagged with the right version due to some issues with mapping the version to the image.
If you run into this issue, you can use the SHA value of the image directly to skip this lookup.
In that case, the image string you would use in the YAML configuration files would be of the form ``<registry>/<image>@sha256:<SHA>``.
The SHA is visible when you push the image to the registry or you can get it by inspecting the image:

.. code-block:: console

    $ docker inspect --format='{{index .RepoDigests 0}}' <registry>/<image>

Prepare the image for Docker deployment
---------------------------------------

Once you build the basic deployment image or pull a pre-built version, you need to prepare it before it can be deployed on Docker.
While the image has the dependencies needed to run the server, there are no models present and the default environment may not be appropriate.
You need to modify this image for the use case that you're targeting.
There are multiple ways of modifying an existing Docker image that are well-documented online and functionally equivalent.
For example, one approach is using a Dockerfile to build a new image:

.. code-block:: dockerfile

    FROM <image>

    COPY /some/path/in/Docker/build/tree /some/path/in/the/image
    ENV SomeVariable=SomeValue
    CMD ["amdinfer-server", "<more arguments?>"]

In this case, you can build and save a new image that includes the models you want to serve.
Depending on the platform, you may need to include other files as well that the platform runtime needs.
As with the deployment image, this image may need to be :ref:`pushed to a registry server <docker~push to a registry>` for your use case.

Note that the command that the image runs and its environment can also be overridden at the command-line when starting the container.
Therefore, an alternative approach to building a new image is to mount the needed files as volumes when starting the container and set the environment then.

Start the container
-------------------

You can start the deployment image with ``docker`` as any other container.
You need to pass along any devices that you want to enable in your container and expose ports to access the server.
Look at the ``docker run`` documentation for more information about what flags can be passed.

.. code-block:: console

    $ docker run [--device ...] [--publish ...] [--volume ...] [--env ...] <image>

By default, the deployment container starts the server executable and it continues to run after the ``docker run`` command.
But before it can serve requests, you need to load the models that you added into the image.
The easiest way to communicate with the server is using the :ref:`Python library <python:install the python library>`.
You can install it locally or use it in the development container to load the workers on the server.

.. code-block:: python

    import amdinfer

    client = amdinfer.HttpClient("http://hostname:port")

    # depending on the model, you need to use the appropriate worker
    worker_name = "migraphx"

    parameters = amdinfer.ParameterMap()
    # specifies the path to the model on the server for it to open
    parameters.put("model", "/path/to/model")

    # workers may accept other parameters at load-time. Refer to worker documentation

    endpoint = client.workerLoad(worker_name, parameters)
    print(endpoint)
    amdinfer.waitUntilModelReady(client, endpoint)

Clients that make requests to this worker need this endpoint to talk to it.

Make a request
--------------

As in the :ref:`Python examples <example_resnet50_python:running resnet50 - python>`, you can make a request by creating a client in Python by pointing it to the address of the server to communicate with.
Unlike these examples, you can skip ahead to making the request for inference because the server is already started and the worker is ready to serve your request.
Once you have it, you can use ``modelInfer`` to make the request.
