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
import proteus

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
            response = self.rest_client.modelInfer(self.model, request)
        except ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        num_inputs = len(request.getInputs())

        if check_asserts:
            assert not response.isError(), response.getError()
            assert response.id == ""
            assert response.model == "PTModel"
            outputs = response.getOutputs()
            assert len(outputs) == num_inputs
            for index, output in enumerate(outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == proteus.DataType.FP32
                assert output.parameters.empty()
        return response

    @pytest.mark.parametrize("num", [1, 8])
    def test_ptzendnn_0(self, num):
        """
        Send a request to pt model as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {"input_size": 224}

        batch = num
        images = []
        for _ in range(batch):
            images.append(
                preprocess_pt(
                    image_path,
                    input_size=preprocessing["input_size"],
                )
            )
        request = proteus.ImageInferenceRequest(images, True)
        response = self.send_request(request)
        top_k_responses = postprocess(response, 5)
        gold_response_output = [259, 157, 152, 261, 154]
        for top_k in top_k_responses:
            assert (top_k == gold_response_output).all()

    @pytest.mark.benchmark(group="PtZendnn")
    def test_benchmark_xmodel(self, benchmark, model_fixture, parameters_fixture):

        batch_size = 16
        input_size = parameters_fixture.get("input_size")
        images = np.random.uniform(
            0.0, 255.0, (batch_size, input_size, input_size, 3)
        ).astype(np.float32)
        images = [image for image in images]

        request = proteus.ImageInferenceRequest(images, True)

        options = {
            "model": model_fixture,
            "parameters": parameters_fixture,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "PtZendnn", self.rest_client.modelInfer, request, **options
        )
