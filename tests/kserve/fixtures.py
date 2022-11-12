# Copyright 2022 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import kserve
import pytest
from kserve import (
    V1alpha1ModelSpec,
    V1alpha1TrainedModel,
    V1alpha1TrainedModelSpec,
    V1beta1InferenceService,
    V1beta1InferenceServiceSpec,
)
from kubernetes.client import V1ObjectMeta

import utils


@pytest.fixture
def create_runtime_isvc(request, runtime_config):
    runtime_key, model_name, storage_uri = request.param

    namespace = runtime_config["kserve"]["namespace"]
    runtime = runtime_config["kserve"][runtime_key]
    storage_uri = runtime_config["kserve"][storage_uri]

    model = model_name
    predictor = utils.get_runtime_predictor(runtime, storage_uri)

    service_name = f"isvc-amdserver-runtime"
    isvc = V1beta1InferenceService(
        api_version=kserve.constants.KSERVE_V1BETA1,
        kind=kserve.constants.KSERVE_KIND,
        metadata=V1ObjectMeta(name=service_name, namespace=namespace),
        spec=V1beta1InferenceServiceSpec(predictor=predictor),
    )

    client = utils.get_kserve_client()
    client.create(isvc)
    print("created kserve service")

    def clean():
        client.delete(service_name, namespace)

    request.addfinalizer(clean)

    with utils.timeout(60, "Inference service didn't become ready in time"):
        client.wait_isvc_ready(service_name, namespace=namespace)
    print("service ready")

    cluster_ip = utils.get_cluster_ip()
    with utils.timeout(300, "Model didn't become ready in time"):
        client.wait_model_ready(
            service_name,
            model,
            isvc_namespace=namespace,
            isvc_version=kserve.constants.KSERVE_V1BETA1_VERSION,
            protocol_version="v2",
            cluster_ip=cluster_ip,
        )
    print("model ready")

    return service_name, model


@pytest.fixture
def create_container_isvc(request, runtime_config):
    image_key = request.param

    namespace = runtime_config["kserve"]["namespace"]
    image = runtime_config["kserve"][image_key]

    service_name = f"isvc-amdserver-mms"
    predictor = utils.get_container_predictor(image)
    isvc = V1beta1InferenceService(
        api_version=kserve.constants.KSERVE_V1BETA1,
        kind=kserve.constants.KSERVE_KIND,
        metadata=V1ObjectMeta(name=service_name, namespace=namespace),
        spec=V1beta1InferenceServiceSpec(predictor=predictor),
    )

    client = utils.get_kserve_client()
    client.create(isvc)
    print("created kserve service")

    def clean():
        client.delete(service_name, namespace)

    request.addfinalizer(clean)

    with utils.timeout(60, "Inference service didn't become ready in time"):
        client.wait_isvc_ready(service_name, namespace=namespace)
    print("service ready")

    return service_name


@pytest.fixture
def create_trained_model(request, create_container_isvc, runtime_config):
    model_name, storage_uri = request.param

    storage_uri = runtime_config["kserve"][storage_uri]
    namespace = runtime_config["kserve"]["namespace"]

    client = utils.get_kserve_client()
    service_name = create_container_isvc

    model_names = [
        model_name,
    ]

    for model_name in model_names:
        model_spec = V1alpha1ModelSpec(
            storage_uri=storage_uri,
            memory="128Mi",
            framework="tensorflow",
        )

        model = V1alpha1TrainedModel(
            api_version=kserve.constants.KSERVE_V1ALPHA1,
            kind=kserve.constants.KSERVE_KIND_TRAINEDMODEL,
            metadata=V1ObjectMeta(name=model_name, namespace=namespace),
            spec=V1alpha1TrainedModelSpec(
                inference_service=service_name, model=model_spec
            ),
        )

        # Create instances of trained models using model1 and model2
        client.create_trained_model(model, namespace)
        print("created trainedmodel")

        def clean():
            # Clean up inference service and trained models
            for model_name in model_names:
                client.delete_trained_model(model_name, namespace)
            print("deleted trainedmodel(s)")

        request.addfinalizer(clean)

        cluster_ip = utils.get_cluster_ip()
        with utils.timeout(300, "Model didn't become ready in time"):
            client.wait_model_ready(
                service_name,
                model_name,
                isvc_namespace=namespace,
                isvc_version=kserve.constants.KSERVE_V1BETA1_VERSION,
                protocol_version="v2",
                cluster_ip=cluster_ip,
            )

    return model_names
