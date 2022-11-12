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

import pytest

kubernetes = pytest.importorskip("kubernetes")
np = pytest.importorskip("numpy")
kserve = pytest.importorskip("kserve")
cv2 = pytest.importorskip("cv2")

import proteus
import proteus.testing

import utils
from fixtures import create_container_isvc, create_runtime_isvc, create_trained_model


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


def make_prediction(model_names, service_name, namespace):
    data = make_request()

    print("making prediction")
    responses = [
        utils.predict(service_name, data, namespace, model_name=model_name)
        for model_name in model_names
    ]
    print("made prediction")
    return responses


def validate(responses):
    assert np.argmax(responses[0]["outputs"][0]["data"]) == 9


@pytest.mark.parametrize(
    "create_trained_model", [("mnist", "mnist_model")], indirect=True
)
@pytest.mark.parametrize("create_container_isvc", ["image_zendnn"], indirect=True)
def test_mms_mnist_kserve(runtime_config, create_container_isvc, create_trained_model):
    namespace = runtime_config["kserve"]["namespace"]

    service_name = create_container_isvc
    model_names = create_trained_model

    responses = make_prediction(model_names, service_name, namespace)
    validate(responses)


@pytest.mark.parametrize(
    "create_runtime_isvc", [("runtime_zendnn", "mnist", "mnist_model")], indirect=True
)
def test_runtime_mnist_kserve(
    runtime_config,
    create_runtime_isvc,
):
    namespace = runtime_config["kserve"]["namespace"]

    service_name, model_name = create_runtime_isvc
    model_names = [model_name]

    responses = make_prediction(model_names, service_name, namespace)
    validate(responses)
