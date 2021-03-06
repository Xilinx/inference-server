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

import json

import pytest

from helper import root_path
import proteus
from proteus.predict_api import InferenceRequestInput, InferenceRequest


@pytest.fixture(scope="class")
def model_fixture():
    return "Resnet50Stream"


@pytest.fixture(scope="class")
def parameters_fixture():
    return None


@pytest.mark.extensions(["aks", "vitis"])
@pytest.mark.fpgas("DPUCADF8H", 1)
@pytest.mark.usefixtures("load")
class TestResnet50Stream:
    """
    Test the Resnet50Stream
    """

    def construct_request(self, requested_frames_count):
        video_path = str(root_path / "tests/assets/Physicsworks.ogv")

        input_0 = InferenceRequestInput()
        input_0.name = "input0"
        input_0.datatype = proteus.DataType.STRING
        input_0.setStringData(video_path)
        input_0.shape = [len(video_path)]
        parameters = proteus.RequestParameters()
        parameters.put("count", requested_frames_count)
        input_0.parameters = parameters

        request = InferenceRequest()
        request.addInputTensor(input_0)
        parameters_2 = proteus.RequestParameters()
        parameters_2.put("key", "0")
        request.parameters = parameters_2

        self.ws_client.modelInferAsync(self.model, request)
        response_str = self.ws_client.modelRecv()
        response = json.loads(response_str)

        assert response["key"] == "0"
        assert float(response["data"]["img"]) == 15.0

    def recv_frames(self, count):
        for i in range(count):
            resp_str = self.ws_client.modelRecv()
            resp = json.loads(resp_str)
            foo = resp["data"]["img"]
            if len(foo) > 50:
                print(f"{i} got an image")
            else:
                print(resp)
            resp["data"]["img"].split(",")[1]

    def test_resnet50stream_0(self):
        requested_frames_count = 100
        self.construct_request(requested_frames_count)
        self.recv_frames(requested_frames_count)
        self.ws_client.close()

    # ? This is commented out because we need a function like run_benchmark()
    # ? to work with the pedantic mode to add the necessary metadata for the
    # ? benchmark to work with the benchmarking framework.
    # @pytest.mark.benchmark(group="resnet50stream")
    # def test_benchmark_resnet50stream_0(self, benchmark):
    #     requested_frames_count = 400
    #     self.construct_request(requested_frames_count)
    #     benchmark.pedantic(self.recv_frames, args=(requested_frames_count,), iterations=1, rounds=1)
