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

import json
import os
import signal
from urllib.parse import urlparse

import kserve
import kubernetes
import requests


def get_kserve_client():
    return kserve.KServeClient(
        config_file=os.environ.get("KUBECONFIG", "~/.kube/config")
    )


def get_cluster_ip():
    """
    Get the kubernetes cluster's IP. This is taken from the KServe repository.

    Returns:
        str: IP of the cluster
    """
    api_instance = kubernetes.client.CoreV1Api(kubernetes.client.ApiClient())
    service = api_instance.read_namespaced_service(
        "istio-ingressgateway", "istio-system"
    )
    if service.status.load_balancer.ingress is None:
        cluster_ip = service.spec.cluster_ip
    else:
        if service.status.load_balancer.ingress[0].hostname:
            cluster_ip = service.status.load_balancer.ingress[0].hostname
        else:
            cluster_ip = service.status.load_balancer.ingress[0].ip
    return os.environ.get("KSERVE_INGRESS_HOST_PORT", cluster_ip)


def get_container_predictor(image, resources):
    limits = {"cpu": "1", "memory": "2Gi"}
    limits.update(resources)

    # Define an inference service
    predictor = kserve.V1beta1PredictorSpec(
        min_replicas=1,
        containers=[
            kubernetes.client.V1Container(
                name="custom",
                image=image,
                args=[
                    "proteus-server",
                    "--http-port=8080",
                    "--grpc-port=9000",
                    "--model-repository=/mnt/models",
                ],
                resources=kubernetes.client.V1ResourceRequirements(
                    requests={"cpu": "1", "memory": "2Gi"},
                    limits=limits,
                ),
                env=[
                    kubernetes.client.V1EnvVar(name="MULTI_MODEL_SERVER", value="true")
                ],
                ports=[
                    kubernetes.client.V1ContainerPort(
                        container_port=8080, protocol="TCP"
                    )
                ],
                # KServe seems to reject livenessProbe, readinessProbe, startupProbe
                # type arguments but it's possible I misconfigured them
            ),
        ],
    )
    return predictor


def get_runtime_predictor(runtime, storage_uri, model_format):
    predictor = kserve.V1beta1PredictorSpec(
        min_replicas=1,
        model=kserve.V1beta1ModelSpec(
            model_format=kserve.V1beta1ModelFormat(
                name=model_format,
            ),
            runtime=runtime,
            storage_uri=storage_uri,
            ports=[
                kubernetes.client.V1ContainerPort(protocol="TCP", container_port=8080)
            ],
        ),
    )
    return predictor


class timeout:
    def __init__(self, seconds=1, error_message="Timeout"):
        self.seconds = seconds
        self.error_message = error_message

    def handle_timeout(self, signum, frame):
        raise TimeoutError(self.error_message)

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.seconds)

    def __exit__(self, type, value, traceback):
        signal.alarm(0)


def predict(
    service_name,
    data,
    namespace,
    model_name=None,
):

    if isinstance(data, dict):
        input_json = json.dumps(data)
    elif isinstance(data, os.PathLike):
        with open(data, "r") as f:
            input_json = json.dumps(json.load(f))
    else:
        raise ValueError(f"Unknown data passed to predict of type {type(data)}")

    # the host is needed for Kubernetes routing
    kfs_client = get_kserve_client()
    isvc = kfs_client.get(
        service_name,
        namespace=namespace,
        version=kserve.constants.KSERVE_V1BETA1_VERSION,
    )
    host = urlparse(isvc["status"]["url"]).netloc
    headers = {"Host": host}

    if model_name is None:
        model_name = service_name

    # only KServe's v2 inference protocol is supported
    cluster_ip = get_cluster_ip()
    url = f"http://{cluster_ip}/v2/models/{model_name}/infer"

    response = requests.post(url, input_json, headers=headers)
    try:
        predictions = json.loads(response.content.decode("utf-8"))
    except json.JSONDecodeError:
        print("Response: ", response.status_code, response.content)
        raise

    return predictions
