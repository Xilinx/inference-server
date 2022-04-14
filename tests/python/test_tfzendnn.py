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
import sys
import json

import cv2
import pytest
import numpy as np
from proteus.predict_api import Datatype, InferenceRequest, RequestInput, RequestOutput
from proteus.exceptions import ConnectionError
from proteus.predict_api import Datatype, ImageInferenceRequest

from helper import run_benchmark, root_path

sys.path.insert(0, os.path.join(root_path, "examples/python"))
from utils.utils import preprocess, postprocess


@pytest.fixture(scope="class")
def model_fixture():
    return "TfZendnn"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {
        "model": str(
            root_path / "external/tensorflow_models/resnet_v1_50_inference.pb"
        ),
        "input_node": "input",
        "output_node": "resnet_v1_50/predictions/Reshape_1",
        "input_size": 224,
        "output_classes": 1000,
        "inter_op": 64,
        "intra_op": 1,
    }


@pytest.mark.usefixtures("load")
class TestTfZendnn:
    """
    Test the TfZendnn worker
    """

    def send_request(self, request, check_asserts=True):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (InferenceRequest): request to send to the server
            output (np.ndarray): Output to check against golden output
            check_asserts (bool): Verify image against golden

        Returns:
            Response: Response as a dictionary
        """

        try:
            response = self.rest_client.infer(self.model, request)
        except ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        num_inputs = len(request.inputs)

        if check_asserts:
            assert not response.error, response.error_msg
            assert response.id == ""
            assert response.model_name == "TFModel"
            assert len(response.outputs) == num_inputs
            for index, output in enumerate(response.outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == Datatype.FP32
                assert output.parameters == {}
        return response

    def test_tfzendnn_0(self):
        """
        Send a request to tf model as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {"input_size": 224, "resize_method": "crop"}

        batch = 1
        images = []
        for _ in range(batch):
            images.append(
                preprocess(
                    image_path,
                    input_size=preprocessing["input_size"],
                    resize_method=preprocessing["resize_method"],
                )
            )
        request = ImageInferenceRequest(images, True)
        response = self.send_request(request)
        k = postprocess(response, 5)
        gold_response_output = [259, 261, 154, 260, 263]
        assert (k == gold_response_output).all()

    def test_tfzendnn_1(self):
        """
        Send a request to tf model as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {"input_size": 224, "resize_method": "crop"}

        batch = 8
        images = []
        for _ in range(batch):
            images.append(
                preprocess(
                    image_path,
                    input_size=preprocessing["input_size"],
                    resize_method=preprocessing["resize_method"],
                )
            )
        request = ImageInferenceRequest(images, True)
        response = self.send_request(request)
        k = postprocess(response, 5)
        gold_response_output = [259, 261, 154, 260, 263]
        assert (k == gold_response_output).all()

    @pytest.mark.benchmark(group="TfZendnn")
    def test_benchmark_xmodel(self, benchmark, model_fixture, parameters_fixture):

        batch_size = 16
        input_size = parameters_fixture.get("input_size")
        images = np.random.uniform(
            0.0, 255.0, (batch_size, input_size, input_size, 3)
        ).astype(np.float32)
        images = [image for image in images]

        request = ImageInferenceRequest(images, True)

        options = {
            "model": model_fixture,
            "parameters": parameters_fixture,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "TfZendnn", self.rest_client._infer, request, **options
        )
