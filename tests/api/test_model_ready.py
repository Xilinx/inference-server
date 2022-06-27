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

import pytest
import time

import proteus


@pytest.mark.usefixtures("server")
class TestModelReady:
    """
    Base class for modelReady tests using the Python bindings
    """

    def is_ready(self, worker):
        try:
            return self.rest_client.modelReady(worker)
        except proteus.Error:
            return False

    def test_model_ready(self):
        """
        Test that modelReady correctly throws errors and
        """

        worker = "echo"

        models = self.rest_client.modelList()
        assert len(models) == 0

        with pytest.raises(proteus.Error) as e_info:
            self.rest_client.modelReady(worker)
            assert str(e_info.value) == f"worker {worker} not found"

        endpoint_0 = self.rest_client.workerLoad(worker)
        assert endpoint_0 == "echo"
        while not self.is_ready(worker):
            time.sleep(1)

        models = self.rest_client.modelList()
        assert len(models) == 1

        self.rest_client.modelUnload(worker)

        while self.is_ready(worker):
            time.sleep(1)

        models = self.rest_client.modelList()
        assert len(models) == 0
