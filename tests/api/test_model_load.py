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


@pytest.mark.extensions(["tfzendnn"])
@pytest.mark.usefixtures("server", "assign_client")
class TestModelLoad:
    def test_model_load(self):
        """
        Test loading multiple times
        """

        model = "mnist"
        models = self.rest_client.modelList()
        assert len(models) == 0

        self.rest_client.modelLoad(model)
        assert self.rest_client.modelReady(model)

        self.rest_client.modelUnload(model)

        amdinfer.waitUntilModelNotReady(self.rest_client, model)


@pytest.mark.extensions(["tfzendnn"])
@pytest.mark.usefixtures("server", "assign_client")
class TestModelLoadVersioned:
    def test_model_load(self):
        """
        Test loading multiple times
        """

        model = "mnist"
        models = self.rest_client.modelList()
        assert len(models) == 0

        version = "1"
        self.rest_client.modelLoad(model, amdinfer.ParameterMap(), version)
        assert self.rest_client.modelReady(model, version)

        self.rest_client.modelUnload(model, version)

        amdinfer.waitUntilModelNotReady(self.rest_client, model, version)
