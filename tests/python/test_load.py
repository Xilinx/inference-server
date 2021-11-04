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

import time


class TestLoad:
    """
    Base class for Echo worker tests
    """

    def test_loads(self):
        """
        Test loading multiple times
        """

        response = self.rest_client.load("echo")  # this loads the model
        assert not response.error, response.error_msg
        endpoint_0 = response.html
        assert endpoint_0 == "echo"
        response = self.rest_client.load("echo")  # this will do nothing and return 200
        assert not response.error, response.error_msg
        assert response.html == "echo"

        response = self.rest_client.load(
            "echo", {"max_buffer_num": 100}
        )  # load echo with a different config
        assert not response.error, response.error_msg
        endpoint_1 = response.html
        assert endpoint_1 == "echo-0"

        # load echo with the same config as earlier but force allocation of a new worker
        response = self.rest_client.load(
            "echo", {"max_buffer_num": 100, "share": False}
        )
        assert not response.error, response.error_msg
        endpoint_1 = response.html
        assert endpoint_1 == "echo-0"

        assert self.rest_client.model_ready(endpoint_0)
        assert self.rest_client.model_ready(endpoint_1)

        self.rest_client.unload(endpoint_0)  # this unloads the model echo
        self.rest_client.unload(endpoint_0)  # this will do nothing and return 200
        self.rest_client.unload(endpoint_1)  # this unloads the model echo-0
        self.rest_client.unload(endpoint_1)  # this unloads the second echo-0 worker

        while self.rest_client.model_ready(endpoint_0):
            time.sleep(1)
        while self.rest_client.model_ready(endpoint_1):
            time.sleep(1)
