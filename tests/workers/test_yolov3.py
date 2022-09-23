# Copyright 2021 Xilinx, Inc.
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

import json
import os

import numpy as np
import pytest
from helper import root_path, run_benchmark

import proteus


@pytest.mark.extensions(["aks", "vitis"])
@pytest.mark.fpgas("DPUCADF8H", 1)
@pytest.mark.usefixtures("load")
class TestInferImageYoloV3DPUCADF8H:
    """
    Test the yolov3 worker
    """

    model = "AksDetect"
    parameters = {
        "aks_graph_name": "yolov3",
        "aks_graph": "${AKS_ROOT}/graph_zoo/graph_yolov3_u200_u250_proteus.json",
    }

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
            response = self.rest_client.modelInfer(self.endpoint, request)
        except proteus.ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        num_inputs = len(request.getInputs())
        # for this picture and xmodel combination, we expect the following output with Vitis 2.5
        gold_response_output = [
            1,
            0.9823938,
            200.2854,
            213.84378,
            178.46155,
            107.93164,
            14,
            0.99329937,
            240.7067,
            154.99591,
            90.769196,
            142.7096,
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
                assert np.allclose(data, gold_response_output, 0.01, 0)
        return response

    def construct_request(self, asTensor):
        image_path = str(root_path / "tests/assets/bicycle-384566_640.jpg")

        batch = 1
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
    def test_benchmark_yolov3_dpucadf8h_1(self, benchmark):
        request = self.construct_request(False)
        options = {
            "model": self.model,
            "parameters": self.parameters,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "yolov3_1", self.rest_client.modelInfer, request, **options
        )
