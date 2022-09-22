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

import numpy as np
import pytest
from proteus.predict_api import InferenceRequest, InferenceRequestInput

import proteus
import proteus.client_operators


@pytest.fixture(scope="class")
def model_fixture():
    return "echoMulti"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {"batch_size": 2, "timeout": 1000}


@pytest.mark.usefixtures("load")
class TestEchoMulti:
    """
    Test the EchoMulti worker
    """

    inputs = [[3], [2, 7]]
    golden_outputs = [[3], [2, 7, 3, 2], [7, 3, 2]]

    @classmethod
    def construct_request(cls):
        """
        Construct a standard request

        Returns:
            dict: The constructed request
        """

        request = InferenceRequest()
        for i in cls.inputs:
            input_0 = InferenceRequestInput()
            input_0.name = "echoMulti0"
            input_0.datatype = proteus.DataType.UINT32
            input_0.shape = [len(i)]
            input_0.setUint32Data(np.array(i, np.uint32))
            request.addInputTensor(input_0)

        return request

    def send_request(self, request):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (InferenceRequest): Request to send to the server
            input_tensors (int): Number of times the two input tensors are repeated

        Returns:
            dict: Response as a dictionary
        """

        requests = [request] * 2
        try:
            responses = proteus.client_operators.inferAsyncOrdered(
                self.rest_client, self.model, requests
            )
        except ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        for response in responses:
            assert not response.isError(), response.getError()
            assert response.model == "echoMulti"

            outputs = response.getOutputs()
            assert len(outputs) == len(self.golden_outputs)

            for i in range(len(outputs)):
                output = outputs[i]
                golden_output = self.golden_outputs[i]
                if not isinstance(golden_output, list):
                    golden_output = [golden_output]
                data: list = output.getUint32Data()
                assert len(data) == len(golden_output)
                assert (data == golden_output).all(), data
                assert output.datatype == proteus.DataType.UINT32
                assert output.name == "echoMulti0"
                assert output.parameters.empty()
                assert output.shape == [len(golden_output)]

    def test_multi_echo_0(self):
        """
        Send a request to echoMulti
        """
        request = self.construct_request()
        self.send_request(request)
