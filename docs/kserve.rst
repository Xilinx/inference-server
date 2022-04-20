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

.. _kserve:

KServe
======

The Xilinx Inference Server can be used with `KServe <https://github.com/kserve/kserve>`__ to deploy the server on a Kubernetes cluster.

Setting up KServe
-----------------

Install KServe using the `instructions <https://kserve.github.io/website/admin/serverless/>`__ provided by KServe.
We have tested with KServe 0.7 using the standard serverless installation but other versions/configurations may work as well.
Once KServe is installed, verify basic functionality of the cluster using the `basic tutorial <https://kserve.github.io/website/get_started/first_isvc/>`__ provided by KServe.
If this succeeds, KServe should be installed correctly.

If you want to use FPGAs for your inferences, install the `Xilinx FPGA Kubernetes plugin <https://github.com/Xilinx/FPGA_as_a_Service/tree/master/k8s-fpga-device-plugin>`__.
This plugin adds FPGAs as a resource for Kubernetes so you can request them when launching services on your cluster.
You may also wish to install monitoring and tracing tools such as Prometheus, Jaeger, and Grafana to your Kubernetes cluster.
Refer to the documentation for these respective projects on installation details.
The `kube-prometheus <https://github.com/prometheus-operator/kube-prometheus/>`__ project may be a good starting point to install some of these tools.


Building the Server
-------------------

To use with KServe, we will need to build the production container.
The production container is optimized for size and only contains the runtime dependencies of the server to allow for quicker deployments.
Update the ``PROTEUS_DEFAULT_HTTP_PORT`` variable in the top-level ``CMakeLists.txt`` to 8080 prior to building the production container.
This change is needed because KServe (currently) has some limitations around using custom ports and this value is hard-coded in some places.
To build the production container [#f1]_:

.. code-block:: console

    $ ./proteus dockerize --production

The resulting image must be pushed to a Docker registry.
If you don't have access to one, you can start a local registry using `these instructions <https://docs.docker.com/registry/deploying/>`__ from Docker.
Make sure to set up a secure registry if you need access to the registry from more than one host.
Once the image is pushed to the registry, verify that it can be pulled with Docker from all hosts in the Kubernetes cluster.

Starting the Service
--------------------

Services in Kubernetes can be started with YAML configuration files.
KServe provides two CRDs that we will use: ``InferenceService`` and ``TrainedModel``.
A sample configuration file to start the Inference Server is provided below:

.. code-block:: yaml

    ---
    apiVersion: "serving.kserve.io/v1beta1"
    kind: InferenceService
    metadata:
    annotations:
        autoscaling.knative.dev/target: "5"
    labels:
        controller-tools.k8s.io: "1.0"
        app: proteus
    name: proteus
    spec:
    predictor:
        containers:
        - name: custom
            image: registry/image:version
            env:
            - name: MULTI_MODEL_SERVER
                value: "true"
            ports:
            - containerPort: 8080
                protocol: TCP
            resources:
            limits:
                xilinx.com/fpga-xilinx_u250_gen3x16_xdma_shell_3_1-0: 1
    ---

Some comments about this configuration file:

#. The autoscaling target defines how the service should be autoscaled in response to incoming requests. The value of 5 indicates that additional containers should be deployed when the number of concurrent requests exceeds 5.
#. The image string should point to image in the registry that you created earlier. In some cases, Kubernetes may fail to pull the image, even if it's tagged with the right version due to some issues with mapping the version to the image. In these cases, you can use the SHA value of the image directly to skip this lookup. In this case, the image string would be ``registry/image@sha256:<SHA>``.
#. We have requested one U250 FPGA using the Xilinx FPGA plugin here. Depending on your use cases, this value may be different or not present.

This service can be deployed on the cluster using:

.. code-block:: console

    $ kubectl apply -f <path to yaml file>

Next, we will deploy the model we want to run.
This use case takes advantage of the multi-model serving feature of KServe.

.. code-block:: yaml

    ---
    apiVersion: "serving.kserve.io/v1alpha1"
    kind: TrainedModel
    metadata:
    name: xmodel-resnet
    spec:
    inferenceService: proteus
    model:
        framework: pytorch
        storageUri: url/to/resnet/xmodel
        memory: 1Gi
    ---

The string passed to the ``name`` field is significant and should be ``xmodel-<something>`` for running XModels.
The string passed to ``inferenceService`` should match the name used in the InferenceServer YAML.
The value of the ``framework`` key is unimportant but must be one of the values expected by KServe.
As before, we can deploy this using:

.. code-block:: console

    $ kubectl apply -f <path to yaml file>

Making Requests
---------------

The method by which you communicate with your service depends on your Kubernetes cluster configuration.
For example, one way to make requests is to get the address of the INGRESS_HOST and INGRESS_PORT, and then make requests to this URL by setting the ``Host`` header on all requests to your targeted service.
This use case may be needed if your cluster doesn't have a load-balancer and/or DNS enabled.

Once you can communicate with your service, you can make requests to the Inference Server using REST (e.g. with the Python API).
The request will be routed to the server and the response will be returned.
To use the Python API, you can set up a `venv <https://docs.python.org/3/library/venv.html>`__ or `conda env <https://docs.conda.io/en/latest/miniconda.html>`__ with Python 3.6+ and install the package in it.
In your virtual environment, run:

.. code-block:: console

    $ cd ./src/python
    $ pip install .

Then, you can import the ``proteus`` package in Python and make requests using the API.
Refer to the Python examples and the API specification for how to make requests.

.. [#f1] Before building the production container, make sure you have all the xclbins for the FPGAs platforms you're targeting in ``./external/overlaybins/``.The contents of this directory will be copied into the production container so these are available to the final image. In addition, you may need to update the value of the ``XLNX_VART_FIRMWARE`` variable in the Dockerfile to point to the path containing your xclbins (it should point to the actual directory containing these files as nested directories aren't searched).

Debugging
---------

Debugging the inference server with KServe adds some additional complexity.
You may have issues with your KServe installation itself (in which case you need to debug KServe alone until you can `run a basic InferenceService <https://kserve.github.io/website/get_started/first_isvc/>`__).
Once the default KServe example works, then you can begin debugging any inference server specific issues.

Use ``kubectl logs <pod_name> <container>`` to see the logs associated with the failing pod.
You'll need to use ``kubectl get pods`` to get the name of the pods corresponding to the InferenceService you're attempting to debug.
The ``logs`` command will list the containers in this pod (if more than one exist) and prompt you to specify the container whose logs you're interested in.
These logs may have helpful error messages.

You can also directly connect to the inference server container that's running in KServe with Docker.
The easiest way to do this is with the ``proteus`` script in the inference server repository.
You'll need to first connect to the node where the container is running.
On that host:

.. code-block:: console
    # this lists the running Inference Server containers
    $ proteus list

    # get the container ID of the container you want to connect to

    # provide the ID as an argument to the attach command to open a bash shell
    # in the container
    $ proteus attach -n <container ID>

Once in the container, you can find the running ``proteus-server`` executable and then follow the regular debugging guide to debug the inference server.
