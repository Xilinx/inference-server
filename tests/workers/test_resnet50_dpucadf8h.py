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

import numpy as np

from helper import run_benchmark, root_path
import proteus


@pytest.fixture(scope="class")
def model_fixture():
    return "resnet50"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {
        "aks_graph_name": "resnet50",
        "aks_graph": "${AKS_ROOT}/graph_zoo/graph_tf_resnet_v1_50_u200_u250_proteus.json",
    }


@pytest.mark.extensions(["aks", "vitis"])
@pytest.mark.fpgas("DPUCADF8H", 1)
@pytest.mark.usefixtures("load")
class TestInferImageResNet50DPUCADF8H:
    """
    Test the Resnet50 worker
    """

    def send_request(self, request, check_asserts=True):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (dict): request to send to the server
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
        gold_response_output = [259, 261, 154, 260, 204]

        if check_asserts:
            assert not response.isError(), response.getError()
            assert response.id == ""
            assert response.model == "resnet50"
            outputs = response.getOutputs()
            assert len(outputs) == num_inputs
            for index, output in enumerate(outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == proteus.DataType.UINT32
                assert output.parameters.empty()
                assert output.shape == [5, 1, 1]
                data = output.getUint32Data()
                assert len(data) == len(gold_response_output)
                np.testing.assert_almost_equal(gold_response_output, data, 2)
        return response

    def construct_request(self, asTensor, batches=4):
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        # TODO(vishalk): AKS gives a segfault if batch != 4
        images = [image_path] * batches

        return proteus.ImageInferenceRequest(images, asTensor)

    def test_resnet50_dpucadf8h_0(self):
        """
        Send a request to resnet50 as tensor data
        """
        request = self.construct_request(True)
        self.send_request(request)

    def test_resnet50_dpucadf8h_1(self):
        """
        Send a request to resnet50 as base64-encoded data
        """
        request = self.construct_request(False)
        self.send_request(request)

    @pytest.mark.benchmark(group="resnet50_dpucadf8h")
    def test_benchmark_resnet50_dpucadf8h_0(
        self, benchmark, model_fixture, parameters_fixture
    ):
        request = self.construct_request(True)
        options = {
            "model": model_fixture,
            "parameters": parameters_fixture,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "resnet50_0", self.rest_client._infer, request, **options
        )

    @pytest.mark.benchmark(group="resnet50_dpucadf8h")
    def test_benchmark_resnet50_dpucadf8h_1(
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
            benchmark, "resnet50_1", self.rest_client._infer, request, **options
        )
