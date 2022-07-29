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
from proteus.predict_api import (
    InferenceRequest,
    InferenceRequestInput,
    InferenceResponse,
)

import proteus


@pytest.fixture(scope="class")
def model_fixture():
    return "aks"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {"batch_size": 1}


@pytest.mark.usefixtures("load")
@pytest.mark.extensions(["aks", "vitis"])
class TestAks:
    def test_aks_0(self):
        numbers = [3.0]
        request = InferenceRequest()
        request.id = "hello_world"

        for num in numbers:
            input_0 = InferenceRequestInput()
            input_0.name = "aks"
            input_0.setFp32Data(np.asarray([num], np.float))
            input_0.shape = [1]
            input_0.datatype = proteus.DataType.FP32
            request.addInputTensor(input_0)
        try:
            response: InferenceResponse = self.rest_client.modelInfer(
                self.model, request
            )
        except ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        assert not response.isError(), response.getError()

        assert response.id == "hello_world"
        assert response.model == "aks"

        outputs = response.getOutputs()
        assert len(outputs) == len(numbers)
        for index, num in enumerate(numbers):
            reply = outputs[index]
            data = reply.getFp32Data()
            assert len(data) == 1
            assert reply.name == "aks"
            assert data[0] == num + 60
            assert reply.datatype == proteus.DataType.FP32
            assert reply.parameters.empty()
