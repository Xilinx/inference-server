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

import proteus.testing


@pytest.fixture
def kserve_client():
    return KServeClient(config_file=os.environ.get("KUBECONFIG", "~/.kube/config"))


@pytest.fixture
def create_inference_service(kserve_client, runtime_config):
    container = runtime_config["kserve"]["zendnn_container"]
    namespace = runtime_config["kserve"]["namespace"]

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

    service_name = f"isvc-amdserver-mms"
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

    yield service_name

    kserve_client.delete(service_name, namespace)


@pytest.fixture
def create_trained_model(kserve_client, create_inference_service, runtime_config):
    storage_uri = runtime_config["kserve"]["mnist_model"]
    namespace = runtime_config["kserve"]["namespace"]

    service_name = create_inference_service

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


def test_mms_amdserver_kserve(
    runtime_config, create_inference_service, create_trained_model
):
    namespace = runtime_config["kserve"]["namespace"]

    service_name = create_inference_service
    model_names = create_trained_model

    data = proteus.testing.getPathToAsset("asset_mnist_v2.json")

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
