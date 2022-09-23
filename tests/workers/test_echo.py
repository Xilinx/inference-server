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

import numpy as np
import pytest
from helper import run_benchmark
from proteus.predict_api import (
    InferenceRequest,
    InferenceRequestInput,
    InferenceRequestOutput,
)

import proteus


@pytest.mark.usefixtures("load")
class TestEcho:
    """
    Test the Echo worker
    """

    model = "echo"
    parameters = None

    inputs = [3]
    golden_outputs = [4]

    @classmethod
    def construct_request(
        cls, add_id, add_input_parameters, add_request_parameters, multiplier=1
    ):
        """
        Construct a standard request subject to some options

        Args:
            add_id (bool): Add an ID to the request
            add_input_parameters (bool): Add parameters to the input tensors
            add_request_parameters (bool): Add parameters to the request

        Returns:
            dict: The constructed request
        """

        input_0 = InferenceRequestInput()
        input_0.name = "echo"
        input_0.datatype = proteus.DataType.UINT32
        input_0.shape = [1]
        input_0.setUint32Data(np.array([cls.inputs[0]], np.uint32))
        if add_input_parameters:
            parameters = proteus.RequestParameters()
            parameters.put("key", "value")
            input_0.parameters = parameters

        request = InferenceRequest()
        for _ in range(multiplier):
            request.addInputTensor(input_0)

        if add_id:
            request.id = "hello_world"

        if add_request_parameters:
            parameters = proteus.RequestParameters()
            parameters.put("key3", True)
            parameters.put("key4", 1.2)
            request.parameters = parameters

        return request

    @staticmethod
    def add_outputs(request: InferenceRequest, add_parameters):
        """
        Add output tensors to the request. This is optional.

        Args:
            request (dict): The request to add outputs to
            add_parameters (bool): Add parameters to the output tensors

        Returns:
            dict: Updated request
        """
        output_0 = InferenceRequestOutput()
        output_0.name = "echo"
        if add_parameters:
            parameters = proteus.RequestParameters()
            parameters.put("key", "value2")
            output_0.parameters = parameters

        request.addOutputTensor(output_0)
        request.addOutputTensor(output_0)
        return request

    def send_request(self, request, id_present, input_tensors=1):
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
                "Connection to the proteus server ended without response!", False
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
            assert output.datatype == proteus.DataType.UINT32
            assert output.name == "echo"
            assert output.parameters.empty()
            assert output.shape == [1]

        if id_present:
            assert response.id == "hello_world"

        return response

    def test_echo_0(self):
        """
        Send a request to echo with all optional parameters
        """
        add_id = True
        request = self.construct_request(add_id, True, True)
        request = self.add_outputs(request, True)

        self.send_request(request, add_id)

    def test_echo_1(self):
        """
        Send a request to echo with most optional parameters (remove request
        parameters)
        """
        add_id = True
        request = self.construct_request(add_id, True, False)
        request = self.add_outputs(request, True)

        self.send_request(request, add_id)

    def test_echo_2(self):
        """
        Send a request to echo with some optional parameters (remove all
        parameters)
        """
        add_id = True
        request = self.construct_request(add_id, False, False)
        request = self.add_outputs(request, False)

        self.send_request(request, add_id)

    def test_echo_3(self):
        """
        Send a request to echo with some optional parameters (remove all
        parameters and remove the ID)
        """
        add_id = False
        request = self.construct_request(add_id, False, False)
        request = self.add_outputs(request, False)

        self.send_request(request, add_id)

    def test_echo_4(self):
        """
        Send a request to echo with no optional parameters (remove all
        parameters, remove the ID and output tensors)
        """
        add_id = False
        request = self.construct_request(add_id, False, False)

        self.send_request(request, add_id)

    @pytest.mark.benchmark(group="echo")
    def test_benchmark_echo_0(self, benchmark):
        request = self.construct_request(False, False, False)
        options = {
            "model": self.model,
            "parameters": self.parameters,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "echo_4", self.rest_client.modelInfer, request, **options
        )


# TODO(varunsh): these tests cannot run now with the new Pybind11 API as it
# doesn't allow creating these kinds of bad requests like this. We'd need to
# reimplement them another way to test these cases
# @pytest.mark.usefixtures("load")
# class TestEchoErrors:
#     """
#     Test the Echo worker with error-causing requests
#     """

#     def send_request(self, request, error):
#         """
#         Send the request to echo and check the error message

#         Args:
#             request (dict): The request to send
#             error (str): Error message to expect
#         """
#         try:
#             response = self.rest_client.modelInfer("echo", request)
#         except ConnectionError:
#             pytest.fail(
#                 "Connection to the proteus server ended without response!", False
#             )

#         assert response.error
#         assert response.status_code == 400
#         assert response.error_msg == error

#     def test_no_inputs(self):
#         """
#         Providing no input tensors is an error
#         """
#         request = {"foo": "bar"}

#         self.send_request(request, "No 'inputs' key present in request")

#     def test_bad_inputs_0(self):
#         """
#         Providing non-array input tensors is an error
#         """
#         request = {"inputs": "bar"}

#         self.send_request(request, "'inputs' is not an array")

#     def test_bad_inputs_1(self):
#         """
#         If inputs is an array, each element must be an object (e.g. a dict)
#         """
#         request = {"inputs": [0]}

#         self.send_request(request, "At least one element in 'inputs' is not an obj")

#     def test_bad_inputs_2(self):
#         """
#         Each input tensor must be named
#         """
#         request = {"inputs": [{"foo": "bar"}], "outputs": []}

#         self.send_request(request, "No 'name' key present in request input")

#     def test_bad_inputs_3(self):
#         """
#         Each input tensor must define its shape
#         """
#         request = {"inputs": [{"name": "bar"}], "outputs": []}

#         self.send_request(request, "No 'shape' key present in request input")

#     def test_bad_inputs_4(self):
#         """
#         An input tensor's shape must be specified with a value that can be
#         coerced into a unint64 value
#         """
#         request = {"inputs": [{"name": "bar", "shape": ["foo"]}], "outputs": []}

#         self.send_request(request, "'shape' must be specified by uint64 elements")

#     def test_bad_inputs_5(self):
#         """
#         All input tensors must define their datatype
#         """
#         request = {"inputs": [{"name": "bar", "shape": [1]}], "outputs": []}

#         self.send_request(request, "No 'datatype' key present in request input")

#     def test_bad_inputs_6(self):
#         """
#         Input tensors' datatypes must be one of the supported values
#         """
#         request = {
#             "inputs": [{"name": "bar", "shape": [1], "datatype": "BAD_TYPE"}],
#             "outputs": [],
#         }

#         self.send_request(request, "Unknown datatype: BAD_TYPE")

#     def test_bad_inputs_7(self):
#         """
#         Input tensors must define their input data
#         """
#         request = {
#             "inputs": [{"name": "bar", "shape": [1], "datatype": "UINT32"}],
#             "outputs": [],
#         }

#         self.send_request(request, "No 'data' key present in request input")
