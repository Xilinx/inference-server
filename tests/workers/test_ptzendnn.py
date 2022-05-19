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

import pytest
import numpy as np
from proteus.predict_api import Datatype
from proteus.exceptions import ConnectionError
from proteus.predict_api import Datatype, ImageInferenceRequest

from helper import run_benchmark, root_path

sys.path.insert(0, os.path.join(root_path, "examples/python"))
from utils.utils import preprocess_pt, postprocess


@pytest.fixture(scope="class")
def model_fixture():
    return "PtZendnn"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {
        "model": str(root_path / "external/pytorch_models/resnet50_pretrained.pt"),
        "input_size": 224,
    }


@pytest.mark.extensions(["ptzendnn"])
@pytest.mark.usefixtures("load")
class TestPtZendnn:
    """
    Test the PtZendnn worker
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
            assert response.model_name == "PTModel"
            assert len(response.outputs) == num_inputs
            for index, output in enumerate(response.outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == Datatype.FP32
                assert output.parameters == {}
        return response

    def test_ptzendnn_0(self):
        """
        Send a request to pt model as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {"input_size": 224}

        batch = 1
        images = []
        for _ in range(batch):
            images.append(
                preprocess_pt(
                    image_path,
                    input_size=preprocessing["input_size"],
                )
            )
        request = ImageInferenceRequest(images, True)
        response = self.send_request(request)
        k = postprocess(response, 5)
        gold_response_output = [259, 157, 152, 261, 154]
        assert (k == gold_response_output).all()

    def test_ptzendnn_1(self):
        """
        Send a request to pt model as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {"input_size": 224}

        batch = 8
        images = []
        for _ in range(batch):
            images.append(
                preprocess_pt(
                    image_path,
                    input_size=preprocessing["input_size"],
                )
            )
        request = ImageInferenceRequest(images, True)
        response = self.send_request(request)
        k = postprocess(response, 5)
        gold_response_output = [259, 157, 152, 261, 154]
        assert (k == gold_response_output).all()

    @pytest.mark.benchmark(group="PtZendnn")
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
            benchmark, "PtZendnn", self.rest_client._infer, request, **options
        )
