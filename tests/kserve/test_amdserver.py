# Copyright 2022 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os

import pytest

kubernetes = pytest.importorskip("kubernetes")
np = pytest.importorskip("numpy")
kserve = pytest.importorskip("kserve")
cv2 = pytest.importorskip("cv2")
from kserve import (
    KServeClient,
    V1alpha1ModelSpec,
    V1alpha1TrainedModel,
    V1alpha1TrainedModelSpec,
    V1beta1CustomPredictor,
    V1beta1InferenceService,
    V1beta1InferenceServiceSpec,
    V1beta1ModelFormat,
    V1beta1ModelSpec,
    V1beta1PredictorSpec,
    V1beta1TransformerSpec,
    constants,
)
from kubernetes.client import (
    V1Container,
    V1ContainerPort,
    V1EnvVar,
    V1ResourceRequirements,
)
from utils import get_cluster_ip, predict

import proteus
import proteus.testing


@pytest.fixture
def kserve_client():
    return KServeClient(config_file=os.environ.get("KUBECONFIG", "~/.kube/config"))


@pytest.fixture
def container_isvc(runtime_config):
    container = runtime_config["kserve"]["zendnn_container"]

    # Define an inference service
    predictor = V1beta1PredictorSpec(
        min_replicas=1,
        containers=[
            V1Container(
                name="custom",
                image=container,
                args=[
                    "proteus-server",
                    "--http-port=8080",
                    "--model-repository=/mnt/models",
                ],
                resources=kubernetes.client.V1ResourceRequirements(
                    requests={"cpu": "50m", "memory": "128Mi"},
                    limits={"cpu": "100m", "memory": "1024Mi"},
                ),
                env=[V1EnvVar(name="MULTI_MODEL_SERVER", value="true")],
                ports=[V1ContainerPort(container_port=8080, protocol="TCP")],
            ),
        ],
    )
    return predictor


@pytest.fixture
def runtime_isvc(runtime_config):
    runtime = runtime_config["kserve"]["runtime"]
    storage_uri = runtime_config["kserve"]["mnist_model"]

    predictor = V1beta1PredictorSpec(
        min_replicas=1,
        model=V1beta1ModelSpec(
            model_format=V1beta1ModelFormat(
                name="tensorflow",
            ),
            runtime=runtime,
            storage_uri=storage_uri,
            ports=[V1ContainerPort(protocol="TCP", container_port=8080)],
        ),
    )
    return predictor, "mnist"


@pytest.fixture
def create_runtime_isvc(kserve_client, runtime_config, runtime_isvc):
    namespace = runtime_config["kserve"]["namespace"]

    predictor, model = runtime_isvc

    service_name = f"isvc-amdserver-runtime"
    isvc = V1beta1InferenceService(
        api_version=constants.KSERVE_V1BETA1,
        kind=constants.KSERVE_KIND,
        metadata=kubernetes.client.V1ObjectMeta(name=service_name, namespace=namespace),
        spec=V1beta1InferenceServiceSpec(predictor=predictor),
    )

    kserve_client.create(isvc)
    print("created kserve service")
    kserve_client.wait_isvc_ready(service_name, namespace=namespace)
    print("service ready")

    cluster_ip = get_cluster_ip()
    kserve_client.wait_model_ready(
        service_name,
        model,
        isvc_namespace=namespace,
        isvc_version=constants.KSERVE_V1BETA1_VERSION,
        protocol_version="v2",
        cluster_ip=cluster_ip,
    )
    print("model ready")

    yield service_name, model

    kserve_client.delete(service_name, namespace)


@pytest.fixture
def create_container_isvc(kserve_client, runtime_config, container_isvc):
    namespace = runtime_config["kserve"]["namespace"]

    service_name = f"isvc-amdserver-mms"
    isvc = V1beta1InferenceService(
        api_version=constants.KSERVE_V1BETA1,
        kind=constants.KSERVE_KIND,
        metadata=kubernetes.client.V1ObjectMeta(name=service_name, namespace=namespace),
        spec=V1beta1InferenceServiceSpec(predictor=container_isvc),
    )

    kserve_client.create(isvc)
    print("created kserve service")
    kserve_client.wait_isvc_ready(service_name, namespace=namespace)
    print("service ready")

    yield service_name

    kserve_client.delete(service_name, namespace)


@pytest.fixture
def create_trained_model(kserve_client, create_container_isvc, runtime_config):
    storage_uri = runtime_config["kserve"]["mnist_model"]
    namespace = runtime_config["kserve"]["namespace"]

    service_name = create_container_isvc

    model_names = [
        "mnist",
    ]

    for model_name in model_names:
        model_spec = V1alpha1ModelSpec(
            storage_uri=storage_uri,
            memory="128Mi",
            framework="tensorflow",
        )

        model = V1alpha1TrainedModel(
            api_version=constants.KSERVE_V1ALPHA1,
            kind=constants.KSERVE_KIND_TRAINEDMODEL,
            metadata=kubernetes.client.V1ObjectMeta(
                name=model_name, namespace=namespace
            ),
            spec=V1alpha1TrainedModelSpec(
                inference_service=service_name, model=model_spec
            ),
        )

        # Create instances of trained models using model1 and model2
        kserve_client.create_trained_model(model, namespace)
        print("created trainedmodel")

        cluster_ip = get_cluster_ip()
        kserve_client.wait_model_ready(
            service_name,
            model_name,
            isvc_namespace=namespace,
            isvc_version=constants.KSERVE_V1BETA1_VERSION,
            protocol_version="v2",
            cluster_ip=cluster_ip,
        )

    yield model_names

    # Clean up inference service and trained models
    for model_name in model_names:
        kserve_client.delete_trained_model(model_name, namespace)
    print("deleted trainedmodel(s)")


def make_request():
    image_path = proteus.testing.getPathToAsset("asset_nine_9723.jpg")
    image = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)
    image = image.astype(np.float32)
    image *= 1.0 / 255.0

    input_0 = proteus.InferenceRequestInput()
    input_0.name = "input-0"
    input_0.datatype = proteus.DataType.FP32
    input_0.shape = [28, 28, 1]
    input_0.setFp32Data(image)

    request = proteus.InferenceRequest()
    request.addInputTensor(input_0)

    return proteus.inference_request_to_dict(request)


def test_mms_amdserver_kserve(
    runtime_config, create_container_isvc, create_trained_model
):
    namespace = runtime_config["kserve"]["namespace"]

    service_name = create_container_isvc
    model_names = create_trained_model

    data = make_request()

    print("making prediction")
    responses = [
        predict(
            service_name,
            data,
            namespace,
            model_name=model_name,
            protocol_version="v2",
        )
        for model_name in model_names
    ]
    print("made prediction")

    assert np.argmax(responses[0]["outputs"][0]["data"]) == 9


def test_runtime_amdserver_kserve(
    runtime_config,
    create_runtime_isvc,
):
    namespace = runtime_config["kserve"]["namespace"]

    service_name, model_name = create_runtime_isvc
    model_names = [model_name]

    data = make_request()

    print("making prediction")
    responses = [
        predict(
            service_name,
            data,
            namespace,
            model_name=model_name,
            protocol_version="v2",
        )
        for model_name in model_names
    ]
    print("made prediction")

    assert np.argmax(responses[0]["outputs"][0]["data"]) == 9
