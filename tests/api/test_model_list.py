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

import pytest


@pytest.mark.usefixtures("server")
class TestModelList:
    """
    Base class for the ModelList tests using the Python bindings
    """

    def test_model_list(self):
        """
        Test listing the running models
        """
        models_0 = self.rest_client.modelList()
        assert len(models_0) == 0

        endpoint = self.rest_client.workerLoad("echo")
        assert endpoint == "echo"
        assert self.rest_client.modelReady(endpoint)

        models = self.rest_client.modelList()
        assert len(models) == 1
        assert models[0] == endpoint

        endpoint_2 = self.rest_client.workerLoad("invertimage")
        assert endpoint_2 == "invertimage"
        assert self.rest_client.modelReady(endpoint_2)

        models = self.rest_client.modelList()
        assert len(models) == 2
        assert endpoint in models
        assert endpoint_2 in models

        self.rest_client.modelUnload(endpoint)
        self.rest_client.modelUnload(endpoint_2)

        models_3 = self.rest_client.modelList()
        while len(models_3) > 0:
            time.sleep(1)
            models_3 = self.rest_client.modelList()
