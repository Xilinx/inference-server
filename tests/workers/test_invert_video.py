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

import cv2
import pytest
from helper import root_path
from proteus.predict_api import InferenceRequest, InferenceRequestInput
from test_invert_image import compare_jpgs

import proteus


@pytest.fixture(scope="class")
def model_fixture():
    return "InvertVideo"


@pytest.fixture(scope="class")
def parameters_fixture():
    return None


@pytest.mark.usefixtures("load")
class TestInvertVideo:
    """
    Test the InvertVideo
    """

    def construct_request(self, video_path, requested_frames_count):
        input_0 = InferenceRequestInput()
        input_0.name = "input0"
        input_0.datatype = proteus.DataType.STRING
        input_0.setStringData(video_path)
        input_0.shape = [len(video_path)]
        parameters = proteus.RequestParameters()
        parameters.put("count", requested_frames_count)
        input_0.parameters = parameters

        # request = WebsocketInferenceRequest(self.model, input_0)
        request = InferenceRequest()
        request.addInputTensor(input_0)
        parameters_2 = proteus.RequestParameters()
        parameters_2.put("key", "0")
        request.parameters = parameters_2

        self.ws_client.modelInferWs(self.model, request)
        response_str = self.ws_client.modelRecv()
        response = json.loads(response_str)

        assert response["key"] == "0"
        assert float(response["data"]["img"]) == 15.0

    def recv_frames(self, video_path, count):

        cap = cv2.VideoCapture(video_path)
        for _ in range(count):
            resp_str = self.ws_client.modelRecv()
            resp = json.loads(resp_str)
            resp_data = resp["data"]["img"].split(",")[1]
            _, frame = cap.read()
            frame = cv2.bitwise_not(frame)
            compare_jpgs(resp_data.encode(), frame)

    def test_invert_video_0(self):
        requested_frames_count = 100
        video_path = str(root_path / "tests/assets/Physicsworks.ogv")

        self.construct_request(video_path, requested_frames_count)
        self.recv_frames(video_path, requested_frames_count)
        self.ws_client.close()

    # ? This is commented out because we need a function like run_benchmark()
    # ? to work with the pedantic mode to add the necessary metadata for the
    # ? benchmark to work with the benchmarking framework.
    # @pytest.mark.benchmark(group="invert_video")
    # def test_benchmark_invert_video_0(self, benchmark):
    #     requested_frames_count = 400
    #     video_path = str(root_path / "tests/assets/Physicsworks.ogv")
    #     self.construct_request(video_path, requested_frames_count)
    #     benchmark.pedantic(self.recv_frames, args=(video_path, requested_frames_count), iterations=1, rounds=1)


# if __name__ == "__main__":
#     from argparse import Namespace

#     client = proteus.clients.HttpClient("http://localhost:8998")
#     models = client.modelList()
#     if "InvertVideo" not in models:
#         worker_name = client.workerLoad("InvertVideo")
#     else:
#         worker_name = "InvertVideo"

#     mod = Namespace()
#     mod.ws_client = proteus.clients.WebSocketClient("ws://localhost:8998", "http://localhost:8998")
#     mod.model = "InvertVideo"

#     runner = TestInvertVideo()
#     runner.ws_client = proteus.clients.WebSocketClient("ws://localhost:8998", "http://localhost:8998")
#     runner.model = "InvertVideo"
#     runner.test_invert_video_0()
