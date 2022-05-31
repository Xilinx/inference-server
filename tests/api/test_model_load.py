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
import time

import proteus


@pytest.mark.usefixtures("server")
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

        endpoint_0 = self.rest_client.modelLoad("echo")  # this loads the model
        assert endpoint_0 == "echo"
        response = self.rest_client.modelLoad(
            "echo"
        )  # this will do nothing and return 200
        assert response == "echo"

        parameters = proteus.RequestParameters()
        parameters.put("max_buffer_num", 100)
        endpoint_1 = self.rest_client.modelLoad(
            "echo", parameters
        )  # load echo with a different config
        assert endpoint_1 == "echo-0"

        # load echo with the same config as earlier but force allocation of a new worker
        parameters.put("share", False)
        response = self.rest_client.modelLoad("echo", parameters)
        assert response == "echo-0"

        assert self.rest_client.modelReady(endpoint_0)
        assert self.rest_client.modelReady(endpoint_1)

        self.rest_client.modelUnload(endpoint_0)  # this unloads the model echo
        self.rest_client.modelUnload(endpoint_0)  # this will do nothing and return 200
        self.rest_client.modelUnload(endpoint_1)  # this unloads the model echo-0
        self.rest_client.modelUnload(
            endpoint_1
        )  # this unloads the second echo-0 worker

        try:
            while self.rest_client.modelReady(endpoint_0):
                time.sleep(1)
        except ValueError:
            pass
        try:
            while self.rest_client.modelReady(endpoint_1):
                time.sleep(1)
        except ValueError:
            pass
