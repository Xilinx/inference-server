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
import proteus.pre_post as pre_post
import proteus.testing

import utils
from fixtures import create_container_isvc, create_runtime_isvc, create_trained_model


def make_request_migraphx():
    image_path = proteus.testing.getPathToAsset("asset_dog-3619020_640.jpg")

    options = pre_post.ImagePreprocessOptionsFloat()
    options.order = pre_post.ImageOrder.NCHW
    options.height = 224
    options.width = 224
    options.mean = [0.485, 0.456, 0.406]
    options.std = [4.367, 4.464, 4.444]
    options.normalize = True
    options.convert_color = True
    options.color_code = cv2.COLOR_BGR2RGB
    options.convert_type = True
    options.type = cv2.CV_32FC3
    options.convert_scale = 1.0 / 255.0
    images = pre_post.imagePreprocessFloat([image_path], options)

    assert len(images) == 1

    input_0 = proteus.InferenceRequestInput()
    input_0.name = "input-0"
    input_0.datatype = proteus.DataType.FP32
    input_0.shape = [224, 224, 3]
    input_0.setFp32Data(images[0])

    request = proteus.InferenceRequest()
    request.addInputTensor(input_0)

    return proteus.inference_request_to_dict(request)


def make_prediction(model_names, service_name, namespace):
    data = make_request_migraphx()

    print("making prediction")
    responses = [
        utils.predict(service_name, data, namespace, model_name=model_name)
        for model_name in model_names
    ]
    print("made prediction")
    return responses


def validate(responses):
    assert len(responses) == 1
    response = responses[0]
    assert len(response["outputs"]) == 1
    output = response["outputs"][0]

    index = pre_post.resnet50PostprocessFloat(output, 1)
    # for this image, this is the expected top classification
    assert index == 259


@pytest.mark.skip("Skipping while debugging the MIGraphX image")
@pytest.mark.parametrize(
    "create_trained_model", [("resnet50-onnx", "resnet50_onnx_model")], indirect=True
)
@pytest.mark.parametrize("create_container_isvc", ["image_migraphx"], indirect=True)
def test_mms_resnet50_kserve(
    runtime_config, create_container_isvc, create_trained_model
):
    namespace = runtime_config["kserve"]["namespace"]

    service_name = create_container_isvc
    model_names = create_trained_model

    responses = make_prediction(model_names, service_name, namespace)
    validate(responses)


@pytest.mark.skip("Skipping while debugging the MIGraphX image")
@pytest.mark.parametrize(
    "create_runtime_isvc",
    [("runtime_migraphx", "resnet50-onnx", "resnet50_onnx_model")],
    indirect=True,
)
def test_runtime_resnet50_kserve(
    runtime_config,
    create_runtime_isvc,
):
    namespace = runtime_config["kserve"]["namespace"]

    service_name, model_name = create_runtime_isvc
    model_names = [model_name]

    responses = make_prediction(model_names, service_name, namespace)
    validate(responses)
