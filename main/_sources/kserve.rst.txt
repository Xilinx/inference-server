..
    Copyright 2021 Xilinx, Inc.
    Copyright 2021 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

KServe
======

You can use the AMD Inference Server with `KServe <https://github.com/kserve/kserve>`__ to deploy the server on a Kubernetes cluster.

Set up Kubernetes and KServe
----------------------------

To use KServe, you will need a Kubernetes cluster.
There are many ways to set up and configure a Kubernetes cluster depending on your use cases.
Instructions for installing and configuring Kubernetes are out of this scope.

Install KServe using the `instructions <https://kserve.github.io/website/admin/serverless/>`__ provided by KServe.
We have tested with KServe 0.8 using the standard serverless installation but other versions/configurations may work as well.
Once KServe is installed, verify basic functionality of the cluster using KServe's `basic tutorial <https://kserve.github.io/website/get_started/first_isvc/>`__.
If this succeeds, KServe should be installed correctly.
KServe installation help and debugging are also out of scope for these instructions.
If you run into problems, reach out to the KServe project.

If you want to use FPGAs for your inferences, install the `Xilinx FPGA Kubernetes plugin <https://github.com/Xilinx/FPGA_as_a_Service/tree/master/k8s-device-plugin>`__.
This plugin adds FPGAs as a resource for Kubernetes so you can request them when launching services on your cluster.

You may also want to install monitoring and tracing tools such as Prometheus, Jaeger, and Grafana to your Kubernetes cluster.
Refer to the documentation for these respective projects on installation details.
The `kube-prometheus <https://github.com/prometheus-operator/kube-prometheus/>`__ project is a good starting point to install some of these tools.


Get or build the AMD Inference Server Image
-------------------------------------------

To use with KServe, you will need to pull or :ref:`build the production container <docker:Build the production Docker image>`.
Once you have it somewhere, make sure you can use ``docker pull <image>`` on all the nodes in the Kubernetes cluster to get the image.

Start an inference service
--------------------------

Services in Kubernetes can be started with YAML configuration files.
To add a service to your cluster, create the configuration file and use ``kubectl apply -f <path to yaml file>``.
KServe provides a number of Custom Resource Definitions (CRDs) that you can use to serve inferences with the AMD Inference Server.
The current recommended approach from KServe is to use the ``ServingRuntime`` method.

As you start an inference service, you will need models to serve.
The model format for the AMD Inference Server is the following:

.. code-block:: text

    /
    ├─ model_a/
    │  ├─ 1/
    │  │  ├─ saved_model.x
    │  ├─ config.pbtxt

The model name, ``model_a`` in this template, must be unique among the models loaded on a particular server.
This name is used to name the endpoint used to make inference requests to.
Under this directory, there must be a directory named ``1/`` containing the model file itself and a text file named ``config.pbtxt``.
The model file must be named ``saved_model`` and the file extension depends on the type of the model.
The ``config.pbtxt`` file contains metadata for the model.
Consider this example of an MNIST TensorFlow model:

.. code-block:: text

    name: "mnist"
    platform: "tensorflow_graphdef"
    inputs [
      {
        name: "images_in"
        datatype: "FP32"
        shape: [28,28,1]
      }
    ]
    outputs [
      {
        name: "flatten/Reshape"
        datatype: "FP32"
        shape: [10]
      }
    ]

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

The inputs and outputs define the list of input and output tensors for the model.
The names of the tensors may be significant if the platform needs them to perform inference.

You can put the model up on any of the cloud storage platforms that KServe supports like GCS, S3 and HTTP.
If you use HTTP, the model should be zipped.
Other archive formats such as ``.tar.gz`` may not work as expected.
Wherever you store it, the URI will be needed to start inference services.

Serving Runtime
^^^^^^^^^^^^^^^

KServe defines two CRDs called ``ServingRuntime`` and ``ClusterServingRuntime``, where the only difference is that the former is namespace-scoped and the latter is cluster-scoped.
You can see more information about these CRDs in `KServe's documentation <https://kserve.github.io/website/0.9/modelserving/servingruntimes/>`__.
The AMD Inference Server is not included by default in the standard KServe installation but you can add the runtime to your cluster.
A sample ``ClusterServingRuntime`` definition is provided below.

.. code-block:: yaml

    ---
    apiVersion: serving.kserve.io/v1alpha1
    kind: ClusterServingRuntime
    metadata:
      # this is the name of the runtime to add
      name: kserve-amdserver
    spec:
      supportedModelFormats:
        # depending on the image you're using, and which platforms are added,
        # the supported formats could be different. For example, this assumes
        # that a ZenDNN image was created with both TF+ZenDNN and PT+ZenDNN
        # support
        - name: tensorflow
          version: "2"
        - name: pytorch
          version: "1"
      protocolVersions:
        # depending on the image you're using, it may not support both HTTP/REST
        # and gRPC, respectively. By default, both protocols are supported.
        - v2
        - grpc-v2
      containers:
        - name: kserve-container
          # provide the image name. The usual rules around images apply (see
          # above in the section "Build the AMD Inference Server Image")
          image: <your image>
          # when the image starts, it will automatically launch the server
          # executable with the following arguments. While the ports used by
          # the server are configurable, there are some assumptions in KServe
          # with the default port values so it is recommended to not change them
          args:
            - proteus-server
            - --model-repository=/mnt/models
            - --enable-repository-watcher
            - --grpc-port=9000
            - --http-port=8080
          # the resources allowed to the service. If the image needs access to
          # hardware like FPGAs or GPUs, then those resources need to be added
          # here so Kubernetes can schedule pods on the appropriate nodes.
          resources:
            requests:
              cpu: "1"
              memory: 2Gi
            limits:
              cpu: "1"
              memory: 2Gi

Adding a ``ClusterServingRuntime`` or a ``ServingRuntime`` is a one-time action per cluster.
Once it's added, you can launch inference services using the runtime like:

.. code-block:: yaml

    ---
    apiVersion: "serving.kserve.io/v1beta1"
    kind: InferenceService
    metadata:
      annotations:
        # The autoscaling target defines how the service should be auto-scale in
        # response to incoming requests. The value of 5 indicates that
        # additional containers should be deployed when the number of concurrent
        # requests exceeds 5.
        autoscaling.knative.dev/target: "5"
      labels:
        controller-tools.k8s.io: "1.0"
        app: example-amdserver-runtime-isvc
      name: example-amdserver-runtime-isvc
    spec:
      predictor:
        model:
          modelFormat:
            name: tensorflow
          storageUri: url/to/model
          # while it's optional for KServe, the runtime should be explicitly
          # specified to make sure the runtime you've added for the AMD Inference
          # Server is used
          runtime: kserve-amdserver

Custom container
^^^^^^^^^^^^^^^^

This approach uses an older method of starting inference services using the ``InferenceService`` and ``TrainedModel`` CRDs, where you start a custom container directly and add models to it.
Initially, no models are loaded on the server as it uses the multi-model serving mechanism of KServe that was a precursor to ModelMesh to support inference servers running multiple models.
Once an ``InferenceService`` is up, you can load models to it by applying one or more ``TrainedModel`` CRDs.
Each such load adds a model to the server and makes it available for inference requests.
A sample YAML file is provided below.

.. code-block:: yaml

    ---
    apiVersion: serving.kserve.io/v1beta1
    kind: InferenceService
    metadata: null
    annotations:
      # The autoscaling target defines how the service should be auto-scaled in
      # response to incoming requests. The value of 5 indicates that additional
      # containers should be deployed when the number of concurrent requests
      # exceeds 5.
      autoscaling.knative.dev/target: '5'
    labels:
      controller-tools.k8s.io: '1.0'
      app: example-amdserver-multi-isvc
    name: example-amdserver-multi-isvc
    spec: null
    predictor:
      containers:
        - name: custom
          image: <your image>
          env:
            - name: MULTI_MODEL_SERVER
              value: 'true'
          args:
            - proteus-server
            - --model-repository=/mnt/models
            - --http-port=8080
            - --grpc-port=9000
          ports:
            - containerPort: 8080
              protocol: TCP
            - containerPort: 9000
              protocol: TCP
    ---
    apiVersion: "serving.kserve.io/v1alpha1"
    kind: TrainedModel
    metadata:
      # this name is significant and must match the top-level directory in the
      # downloaded model at the storageUri. This string becomes the endpoint u
      # used to make inferences
      name: <name of the model>
    spec:
      # the name used here must match an existing InferenceService to load
      # this TrainedModel to
      inferenceService: example-amdserver-multi-isvc
      model:
        framework: tensorflow
        storageUri: url/to/model
        memory: 1Gi

Making Requests
---------------

The method by which you communicate with your service depends on your Kubernetes cluster configuration.
For example, one way to make requests is to `get the address of the INGRESS_HOST and INGRESS_PORT <https://kserve.github.io/website/master/get_started/first_isvc/#4-determine-the-ingress-ip-and-ports>`__, and then make requests to this URL by setting the ``Host`` header on all requests to your targeted service.
This use case may be needed if your cluster doesn't have a load-balancer and/or DNS enabled.

Once you can communicate with your service, you can make requests to the Inference Server using REST with cURL or the `KServe Python API <https://kserve.github.io/website/0.8/sdk_docs/sdk_doc/>`__.
The request will be routed to the server and the response will be returned.
You can see some examples of using the KServe Python API to make requests in the `tests <https://github.com/Xilinx/inference-server/tree/main/tests/kserve>`__.

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

.. code-block:: bash

    # this lists the running Inference Server containers
    proteus list

    # get the container ID of the container you want to connect to

    # provide the ID as an argument to the attach command to open a bash shell
    # in the container
    proteus attach -n <container ID>

Once in the container, you can find the running ``proteus-server`` executable and then follow the regular debugging guide to debug the inference server.
