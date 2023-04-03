# Copyright 2023 Advanced Micro Devices, Inc.
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

import amdinfer


@pytest.mark.usefixtures("load")
class TestCPlusPlus:
    """
    Test the CPlusPlus worker
    """

    @staticmethod
    def get_config():
        model = ["CPlusPlus"]
        parameters = [amdinfer.ParameterMap(["model"], ["echo"])]
        return (model, parameters)

    inputs = [3]
    golden_outputs = [4]

    @classmethod
    def construct_request(cls):
        """
        Construct a standard request subject to some options

        Returns:
            InferenceRequest: The constructed request
        """

        input_0 = amdinfer.InferenceRequestInput()
        input_0.name = "echo"
        input_0.datatype = amdinfer.DataType.UINT32
        input_0.shape = [1]
        input_0.setUint32Data(np.array([cls.inputs[0]], np.uint32))

        request = amdinfer.InferenceRequest()
        request.addInputTensor(input_0)

        return request

    def send_request(self, request, input_tensors=1):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (InferenceRequest): Request to send to the server
            input_tensors (int): Number of times the two input tensors are repeated

        Returns:
            dict: Response as a dictionary
        """

        try:
            response = self.rest_client.modelInfer(self.endpoint, request)
        except ConnectionError:
            pytest.fail(
                "Connection to the amdinfer server ended without response!", False
            )

        assert not response.isError(), response.getError()
        assert response.model == "echo"

        outputs = response.getOutputs()
        assert len(outputs) == 1 * input_tensors

        for i in range(input_tensors):
            output = outputs[i]
            data = output.getUint32Data()
            assert len(data) == 1
            assert data[0] == self.golden_outputs[0]
            assert output.datatype == amdinfer.DataType.UINT32
            assert output.name == ""
            assert output.parameters.empty()
            assert output.shape == [1]

        return response

    def test_c_plus_plus(self):
        """
        Send a request to test c_plus_plus
        """
        request = self.construct_request()
        self.send_request(request)


@pytest.mark.usefixtures("load")
class TestCPlusPlus2:
    """
    Test the CPlusPlus worker
    """

    @staticmethod
    def get_config():
        model = ["CPlusPlus"]
        parameters = [
            amdinfer.ParameterMap(
                ["model", "batch_size", "timeout"], ["echo_multi", 2, 1000]
            )
        ]
        return (model, parameters)

    inputs = [[3], [2, 7]]
    golden_outputs = [[3], [2, 7, 3, 2], [7, 3, 2]]

    @classmethod
    def construct_request(cls):
        """
        Construct a standard request subject to some options

        Returns:
            InferenceRequest: The constructed request
        """

        request = amdinfer.InferenceRequest()
        for i in cls.inputs:
            input_0 = amdinfer.InferenceRequestInput()
            input_0.name = "echoMulti0"
            input_0.datatype = amdinfer.DataType.UINT32
            input_0.shape = [len(i)]
            input_0.setUint32Data(np.array(i, np.uint32))
            request.addInputTensor(input_0)

        return request

    def send_request(self, request, input_tensors=1):
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
            responses = amdinfer.inferAsyncOrdered(
                self.rest_client, self.endpoint, requests
            )
        except ConnectionError:
            pytest.fail(
                "Connection to the amdinfer server ended without response!", False
            )

        for response in responses:
            assert not response.isError(), response.getError()
            assert response.model == "echo_multi"

            outputs = response.getOutputs()
            assert len(outputs) == len(self.golden_outputs)

            for i in range(len(outputs)):
                output = outputs[i]
                golden_output = self.golden_outputs[i]
                if not isinstance(golden_output, list):
                    golden_output = [golden_output]
                data: list = output.getUint32Data()
                assert len(data) == len(golden_output)
                assert output.datatype == amdinfer.DataType.UINT32
                assert output.name == "output" + str(i)
                assert output.parameters.empty()
                assert output.shape == [len(golden_output)]
                print(data)
                assert (data == golden_output).all(), data

    def test_c_plus_plus(self):
        """
        Send a request to test c_plus_plus
        """
        request = self.construct_request()
        self.send_request(request)
