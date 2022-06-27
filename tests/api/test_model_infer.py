# Copyright 2022 Xilinx Inc.
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

import time

import numpy as np
import pytest

import proteus
import proteus.predict_api


@pytest.mark.usefixtures("server")
class TestModelInfer:
    """
    Base class for the ModelInfer tests using the Python bindings
    """

    def test_model_infer(self):
        """
        Test making an inference
        """

        endpoint = self.rest_client.workerLoad("echo")
        assert endpoint == "echo"

        input_data = proteus.predict_api.InferenceRequestInput()
        input_data.shape = [1]
        input_data.setUint32Data(np.array([1], np.uint32))
        request = proteus.predict_api.InferenceRequest()
        request.addInputTensor(input_data)

        response = self.rest_client.modelInfer(endpoint, request)
        assert not response.isError(), response.getError()

        outputs = response.getOutputs()
        assert len(outputs) == 1
        output_data = outputs[0].getUint32Data()
        assert len(output_data) == 1
        assert output_data[0] == 2

        self.rest_client.modelUnload(endpoint)

        try:
            while self.rest_client.modelReady(endpoint):
                time.sleep(1)
        except RuntimeError:
            pass
