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

        parameters = amdinfer.ParameterMap(["model"], ["echo"])
        chain = amdinfer.Chain(["cplusplus"], [parameters])
        chain.load(self.rest_client)
        endpoint = chain.get()
        assert endpoint == "cplusplus"
        assert self.rest_client.modelReady(endpoint)

        models = self.rest_client.modelList()
        assert len(models) == 2
        assert models[0] == endpoint

        parameters = amdinfer.ParameterMap(["model"], ["echo_multi"])
        chain2 = amdinfer.Chain(["cplusplus"], [parameters])
        chain2.load(self.rest_client)
        endpoint_2 = chain2.get()
        assert endpoint_2 == "cplusplus-0"
        assert self.rest_client.modelReady(endpoint_2)

        models = self.rest_client.modelList()
        # one each for echo and echo_multi models + 1 for a shared responder
        assert len(models) == 3
        assert endpoint in models
        assert endpoint_2 in models

        chain.unload(self.rest_client)
        chain2.unload(self.rest_client)

        models_3 = self.rest_client.modelList()
        while len(models_3) > 0:
            time.sleep(1)
            models_3 = self.rest_client.modelList()
