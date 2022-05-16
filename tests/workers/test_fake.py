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
from proteus.predict_api import (
    InferenceRequest,
    InferenceRequestInput,
    InferenceRequestOutput,
)

from helper import run_benchmark


@pytest.fixture(scope="class")
def model_fixture():
    return "fake"


@pytest.fixture(scope="class")
def parameters_fixture():
    return None


# @pytest.mark.usefixtures("load")
# class TestFake:
#     """
#     Test the Fake worker
#     """

#     def test_fake_0(self):
#         """
#         Send a request to echo with no optional parameters with too many input
#         tensors.
#         """
#         input_0 = RequestInput("fake")
#         input_0.datatype = Datatype.UINT32
#         input_0.shape.append(1)
#         input_0.data.append(1)
#         request = InferenceRequest(input_0)
#         response = self.rest_client._infer("fake", request)
#         assert response.status_code == 200
