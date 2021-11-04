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

from proteus.predict_api import RequestInput, Datatype, InferenceRequest


@pytest.fixture(scope="class")
def model_fixture():
    return "aks"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {"batch_size": 1}


@pytest.mark.usefixtures("load")
class TestAks:
    def test_aks_0(self):
        numbers = [3.0, 5.0]
        request = InferenceRequest(id="hello_world")

        for num in numbers:
            input_0 = RequestInput("aks")
            input_0.data = [num]
            input_0.shape = [1]
            input_0.datatype = Datatype.FP32
            request.inputs.append(input_0)

        try:
            response = self.rest_client.infer(self.model, request)
        except ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        assert not response.error, response.error_msg

        assert response.id == "hello_world"
        assert response.model_name == "aks"

        assert len(response.outputs) == len(numbers)
        for index, num in enumerate(numbers):
            reply = response.outputs[index]
            assert len(reply.data) == 1
            assert reply.name == "aks"
            assert reply.data[0] == num + 60
            assert reply.datatype == Datatype.FP32
            assert reply.parameters == {}
