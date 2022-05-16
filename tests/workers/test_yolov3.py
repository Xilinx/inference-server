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


@pytest.fixture(scope="class")
def model_fixture():
    return "AksDetect"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {
        "aks_graph_name": "yolov3",
        "aks_graph": "${AKS_ROOT}/graph_zoo/graph_yolov3_u200_u250_proteus.json",
    }


@pytest.mark.extensions(["aks", "vitis"])
@pytest.mark.fpgas("DPUCADF8H", 1)
@pytest.mark.usefixtures("load")
class TestInferImageYoloV3DPUCADF8H:
    """
    Test the yolov3 worker
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
        # for this picture and xmodel combination, we expect the following output with Vitis 1.4
        gold_response_output = [
            1,
            0.7396191358566284,
            192.97344970703125,
            198.21803283691406,
            167.64910888671875,
            130.1903839111328,
            14,
            0.9992860555648804,
            232.0582733154297,
            148.03860473632812,
            109.48860168457031,
            161.711181640625,
        ]

        # for this picture and xmodel combination, we expect the following output with Vitis 2.0
        gold_response_output_2 = [
            1,
            0.9974043369293213,
            220.02317810058594,
            213.84378051757812,
            138.98597717285156,
            107.931640625,
            14,
            0.9992860555648804,
            232.0582733154297,
            148.03860473632812,
            109.48860168457031,
            161.711181640625,
        ]

        if check_asserts:
            assert not response.isError(), response.getError()
            # content = response.json()
            assert response.id == ""
            assert response.model == "yolov3"
            outputs = response.getOutputs()
            assert len(outputs) == num_inputs
            for index, output in enumerate(outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == proteus.DataType.FP32
                assert output.parameters.empty()
                data = output.getFp32Data()
                num_boxes = int(len(data) / 6)
                assert output.shape == [6, num_boxes]
                assert len(data) == len(gold_response_output)
                assert np.allclose(data, gold_response_output, 0.01, 0) or np.allclose(
                    data, gold_response_output_2, 0.01, 0
                )
        return response

    def construct_request(self, asTensor):
        image_path = str(root_path / "tests/assets/bicycle-384566_640.jpg")

        # TODO(vishalk): AKS gives a segfault if batch != 4
        batch = 4
        images = [image_path] * batch
        request = proteus.ImageInferenceRequest(images, asTensor)

        return request

    def test_yolov3_dpucadf8h_1(self):
        """
        Send a request to yolov3 as base64-encoded data
        """
        request = self.construct_request(False)
        self.send_request(request)

    @pytest.mark.benchmark(group="yolov3_dpucadf8h")
    def test_benchmark_yolov3_dpucadf8h_1(
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
            benchmark, "yolov3_1", self.rest_client._infer, request, **options
        )
