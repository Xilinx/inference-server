# Copyright 2022 Xilinx, Inc.
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

import time

import pytest

import amdinfer


@pytest.mark.usefixtures("server", "assign_client")
class TestModelReady:
    """
    Base class for modelReady tests using the Python bindings
    """

    def test_model_ready(self):
        """
        Test that modelReady correctly throws errors and
        """

        worker = "cplusplus"

        models = self.rest_client.modelList()
        assert len(models) == 0

        assert not self.rest_client.modelReady(worker)

        parameters = amdinfer.ParameterMap(["model"], ["echo"])
        endpoint_0 = self.rest_client.workerLoad(worker, parameters)
        assert endpoint_0 == "cplusplus"
        while not self.rest_client.modelReady(endpoint_0):
            time.sleep(1)

        models = self.rest_client.modelList()
        assert len(models) == 1

        self.rest_client.modelUnload(endpoint_0)

        while self.rest_client.modelReady(endpoint_0):
            time.sleep(1)

        models = self.rest_client.modelList()
        assert len(models) == 0
