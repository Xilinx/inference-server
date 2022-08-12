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

import numpy as np
import pytest
from helper import root_path, run_benchmark

import proteus


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
            response = self.rest_client.modelInfer(self.model, request)
        except proteus.ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        num_inputs = len(request.getInputs())
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
            assert not response.isError(), response.getError()
            assert response.id == ""
            assert response.model == "facedetect"
            outputs = response.getOutputs()
            assert len(outputs) == num_inputs
            for index, output in enumerate(outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == proteus.DataType.FP32
                assert output.parameters.empty()
                data = output.getFp32Data()
                num_boxes = int(len(data) / 6)
                assert output.shape == [6, num_boxes]
                assert len(data) == len(
                    gold_response_output
                ), f"{len(data)} != {len(gold_response_output)}"
                np.testing.assert_almost_equal(gold_response_output, data, 2)
        return response

    def construct_request(self, asTensor):
        image_path = str(root_path / "tests/assets/girl-1867092_640.jpg")
        batch = 1
        images = [image_path] * batch
        request = proteus.ImageInferenceRequest(images, asTensor)

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
