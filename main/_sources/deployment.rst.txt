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

Deployment
==========

Deployment is |define_deployment|.
There are many different ways to deploy the server and the best one will depend on your use cases.
However, you will use a container in all cases.

Deployment image
----------------

The :ref:`deployment image <terminology~Deployment>` is optimized for size and automatically starts the server executable as the container runs.
If you do not need to frequently debug the server or compile the executable in a custom way, using the deployment image provides a self-contained approach to deploy the server on a machine.

Get the image
^^^^^^^^^^^^^

To use the pre-built images for the inference server, you can pull them from DockerHub:

.. include:: dry.rst
    :start-after: +docker_pull_deployment_images
    :end-before: -docker_pull_deployment_images

Build the image
^^^^^^^^^^^^^^^

If you want an image with a custom build of the server or with an arbitrary combination of backends, you will need to build it yourself.
After cloning the inference server repository, you can build the deployment image with:

.. code-block:: console

    $ python3 docker/generate.py
    $ ./amdinfer dockerize --production <backend flags>

Depending on what backends you want to support, add the appropriate flags to enable them.
You can enable multiple backends by passing flags for all the backends you want to enable in the image.
Refer to the help for this command or the :ref:`backend documentation <backends:Backends>` for more information on how to build the right image.

.. important::

    Some backends require additional steps before building the Docker image with them enabled.
    Look at the backend-specific documentation for more information about how to build images for a given backend.

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

Prepare the image
^^^^^^^^^^^^^^^^^

Once you build the basic deployment image or pull a pre-built version, you may need to prepare it before it can be deployed.
While the image has the dependencies needed to run the server, there are no models present and the default environment may not be appropriate.
You may need to modify this image for the use case that you're targeting.
There are multiple ways of modifying an existing Docker image that are well-documented online and functionally equivalent.
For example, one approach is using a Dockerfile to build a new image:

.. code-block:: dockerfile

    FROM <image>

    COPY /some/path/in/Docker/build/tree /some/path/in/the/image
    ENV SomeVariable=SomeValue
    CMD ["amdinfer-server", "<more arguments?>"]

In this case, you can build and save a new image that includes the additional files you added.
Depending on the backend, you may need to include other files as well that the backend runtime needs.

Note that the command that the image runs and its environment can also be overridden at the command-line when starting the container.
Therefore, an alternative approach to building a new image is to mount the needed files as volumes when starting the container and set the environment then as well.

Start a container
-----------------

You can start a deployment container with ``docker`` as any other container.
You need to pass along any devices that you want to enable in your container and expose ports to access the server.
Look at the ``docker run`` documentation for more information about what flags can be passed.

.. code-block:: console

    $ docker run [--device ...] [--publish ...] [--volume ...] [--env ...] <image>

By default, the deployment container starts the server executable and it continues to run after the ``docker run`` command.
But before it can serve requests, you need to load the models that you added into the image.
You can set up a :ref:`model repository <model_repository:Model Repository>` that gets loaded automatically or manually load backends using the client API ``workerLoad()``.
Look at the :ref:`backend documentation <backends:Backends>` for information on how to load them.

KServe
------

You can deploy the deployment image with KServe onto a Kubernetes cluster.
See the :ref:`KServe guide <kserve:KServe>` for more information.

Development image
-----------------

The :ref:`development image <terminology~Development>` contains all the libraries and tools to compile the server executable.
If you are experimenting with development or debugging, using the development image can provide a way to deploy the server on a machine.
See the :ref:`developer quickstart <quickstart_development:Developer Quickstart>` for how to get started with this image.

Once you have compiled the server, you can start it in the container and send it requests from within the container.
If you are debugging, it can be helpful to start the server with ``gdb``.
The container will also have published ports on the host machine so you can also send requests to your running server executable in the container from the outside using this port.
Use ``docker ps`` to see what the port mapping is.
