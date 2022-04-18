# Copyright 2021 Xilinx Inc.
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
import json
import os
import numpy as np

from helper import run_benchmark, root_path
import proteus
from proteus.predict_api import Datatype, ImageInferenceRequest


@pytest.fixture(scope="class")
def model_fixture():
    return "AksDetect"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {
        "aks_graph_name": "facedetect",
        "aks_graph": "${AKS_ROOT}/graph_zoo/graph_facedetect_u200_u250_proteus.json",
    }


@pytest.mark.extensions(["aks", "vitis"])
@pytest.mark.fpgas("DPUCADF8H", 1)
@pytest.mark.usefixtures("load")
class TestInferImageFacedetectDPUCADF8H:
    """
    Test the facedetect worker
    """

    def send_request(self, request, check_asserts=True):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (InferenceRequest): Request to send to the server
            image (np.ndarray): Golden image to check against
            check_asserts (bool): Verify image against golden

        Returns:
            Response: Response as a dictionary
        """

        try:
            response = self.rest_client.infer(self.model, request)
        except proteus.ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        num_inputs = len(request.inputs)
        # for this picture and xmodel combination, we expect the following output
        gold_response_output = [
            -1,
            0.9937100410461426,
            268,
            79.875,
            156,
            169.06874084472656,
        ]

        if check_asserts:
            # print(response.error_msg)
            assert not response.error, response.error_msg
            # content = response.json()
            assert response.id == ""
            assert response.model_name == "facedetect"
            assert len(response.outputs) == num_inputs
            for index, output in enumerate(response.outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == Datatype.FP32
                assert output.parameters == {}
                num_boxes = int(len(output.data) / 6)
                assert output.shape == [6, num_boxes]
                assert len(output.data) == len(gold_response_output)
                np.testing.assert_almost_equal(gold_response_output, output.data, 2)
        return response

    def construct_request(self, asTensor):
        image_path = str(root_path / "tests/assets/girl-1867092_640.jpg")
        # TODO(vishalk): AKS gives a segfault if batch != 4
        batch = 4
        images = [image_path] * batch
        request = ImageInferenceRequest(images, asTensor)

        return request

    def test_facedetect_dpucadf8h_1(self):
        """
        Send a request to facedetect as base64-encoded data
        """
        request = self.construct_request(False)
        self.send_request(request)

    @pytest.mark.benchmark(group="facedetect_dpucadf8h")
    def test_benchmark_facedetect_dpucadf8h_1(
        self, benchmark, model_fixture, parameters_fixture
    ):
        request = self.construct_request(False)
        options = {
            "model": model_fixture,
            "parameters": parameters_fixture,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "facedetect_1", self.rest_client._infer, request, **options
        )
