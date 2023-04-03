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

import time

import pytest

import amdinfer


@pytest.mark.usefixtures("server", "assign_client")
class TestLoad:
    """
    Base class for Echo worker tests
    """

    def test_loads(self):
        """
        Test loading multiple times
        """

        models = self.rest_client.modelList()
        assert len(models) == 0

        worker = "cplusplus"
        parameters = amdinfer.ParameterMap(["model"], ["echo"])

        # this loads the model
        endpoint_0 = self.rest_client.workerLoad(worker, parameters)
        assert endpoint_0 == worker
        # this will do nothing and return the same endpoint
        endpoint_0 = self.rest_client.workerLoad(worker, parameters)
        assert endpoint_0 == worker

        # load the same worker with a different config
        # arbitrarily set to 100 just to create a different config
        parameters.put("max_buffer_num", 100)
        endpoint_1 = self.rest_client.workerLoad(worker, parameters)
        expected_endpoint = worker + "-0"
        assert endpoint_1 == expected_endpoint

        # load worker with the same config as earlier but force allocation of a new worker
        parameters.put("share", False)
        endpoint_1 = self.rest_client.workerLoad(worker, parameters)
        assert endpoint_1 == expected_endpoint

        assert self.rest_client.modelReady(endpoint_0)
        assert self.rest_client.modelReady(endpoint_1)

        # this unloads the first model
        self.rest_client.modelUnload(endpoint_0)
        # this will do nothing and return
        self.rest_client.modelUnload(endpoint_0)
        # this unloads the one copy of the second worker
        self.rest_client.modelUnload(endpoint_1)
        # this unloads the second copy of the second worker
        self.rest_client.modelUnload(endpoint_1)

        while self.rest_client.modelReady(endpoint_0):
            time.sleep(1)
        while self.rest_client.modelReady(endpoint_1):
            time.sleep(1)
