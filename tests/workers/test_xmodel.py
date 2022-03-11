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
import math

import cv2
import numpy as np

from helper import run_benchmark, root_path
from proteus.predict_api import Datatype, ImageInferenceRequest


@pytest.fixture(scope="class")
def model_fixture():
    return "Xmodel"


@pytest.fixture(scope="class")
def parameters_fixture():
    return None


@pytest.mark.extensions(["vitis"])
@pytest.mark.fpgas("DPUCADF8H", 1)
@pytest.mark.usefixtures("load")
class TestXmodel:
    """
    Test the Xmodel worker
    """

    def send_request(self, request, check_asserts=True):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (InferenceRequest): request to send to the server
            image (np.ndarray): Golden image to check against
            check_asserts (bool): Verify image against golden

        Returns:
            Response: Response as a dictionary
        """

        try:
            response = self.rest_client.infer(self.model, request)
        except ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        num_inputs = len(request.inputs)

        if check_asserts:
            assert not response.error, response.error_msg
            assert response.id == ""
            assert response.model_name == "xmodel"
            assert len(response.outputs) == num_inputs
            for index, output in enumerate(response.outputs):
                assert output.name == "input" + str(index)
                # the default type out of tensors is int64 but doesn't look right...
                # assert output.datatype == Datatype.INT64
                assert output.datatype == Datatype.INT8
                assert output.parameters == {}
        return response

    @staticmethod
    def preprocess(images, preprocessing):
        images_preprocessed = []

        data_type = preprocessing["type"]
        height = preprocessing["net_h"]
        width = preprocessing["net_w"]
        channels = preprocessing["net_c"]
        layout = preprocessing["output_layout"]
        fix_scale = preprocessing["fix_scale"]
        mean = preprocessing["mean"]

        for image in images:
            image = image.astype(getattr(np, data_type))
            image = cv2.resize(image, (height, width))
            new_image = np.zeros(image.shape, dtype=np.int8)
            if layout == "NCHW":  # image defaults to HWC
                image = np.rollaxis(image, axis=2, start=0)  # reshape to CHW
                for c in range(channels):
                    for h in range(height):
                        for w in range(width):
                            new_image[c][h][w] = int(
                                (image[c][h][w] - mean[c]) * fix_scale
                            )

            else:
                for h in range(height):
                    for w in range(width):
                        for c in range(channels):
                            new_image[h][w][c] = int(
                                (image[h][w][c] - mean[c]) * fix_scale
                            )
            images_preprocessed.append(new_image)

        return images_preprocessed

    @staticmethod
    def postprocess(data, k):
        max_val = max(data)
        softmax = [0] * len(data)

        data_sum = 0
        for i, val in enumerate(data):
            softmax[i] = math.exp(val - max_val)
            data_sum += softmax[i]

        for i, val in enumerate(data):
            softmax[i] /= data_sum

        # get the top classifications
        tmp = [
            i[0] for i in sorted(enumerate(softmax), reverse=True, key=lambda x: x[1])
        ]

        # only return the top k
        return tmp[:k]

    def test_xmodel_0(self):
        """
        Send a request to resnet50 as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {
            "net_w": 224,
            "net_h": 224,
            "net_c": 3,
            "mean": [123, 107, 104],
            "fix_scale": 1,
            "output_layout": "NHWC",
            "type": "uint8",
        }

        batch = 4
        images = []
        for _ in range(batch):
            image = cv2.imread(image_path)
            images.append(image)

        images = self.preprocess(images, preprocessing)
        request = ImageInferenceRequest(images, True)
        response = self.send_request(request)
        output = response.outputs[0].data
        k = self.postprocess(output, 5)
        gold_response_output = [259, 261, 260, 154, 230]
        assert k == gold_response_output

    def test_xmodel_1(self):
        """
        Send a request to resnet50 as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {
            "net_w": 224,
            "net_h": 224,
            "net_c": 3,
            "mean": [123, 107, 104],
            "fix_scale": 1,
            "output_layout": "NHWC",
            "type": "uint8",
        }

        batch = 1
        images = []
        for _ in range(batch):
            image = cv2.imread(image_path)
            images.append(image)

        images = self.preprocess(images, preprocessing)
        request = ImageInferenceRequest(images, True)
        requests = [request] * 4
        models = [self.model] * 4
        responses = self.rest_client.infers(models, requests)
        assert len(responses) == 4
        for index, response in enumerate(responses):
            num_inputs = len(requests[index].inputs)

            assert not response.error, response.error_msg
            assert response.id == ""
            assert response.model_name == "xmodel"
            assert len(response.outputs) == num_inputs
            for index, output in enumerate(response.outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == Datatype.INT8
                assert output.parameters == {}
            # response = self.send_request(request)
            output = response.outputs[0].data
            k = self.postprocess(output, 5)
            gold_response_output = [259, 261, 260, 154, 230]
            assert k == gold_response_output

    @pytest.mark.benchmark(group="xmodel")
    def test_benchmark_xmodel(self, benchmark, model_fixture, parameters_fixture):
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")
        print(image_path)

        preprocessing = {
            "net_w": 4,
            "net_h": 4,
            "net_c": 3,
            "mean": [123, 107, 104],
            "fix_scale": 1,
            "output_layout": "NHWC",
            "type": "uint8",
        }

        batch = 4
        images = []
        for _ in range(batch):
            image = cv2.imread(image_path)
            images.append(image)

        images = self.preprocess(images, preprocessing)
        request = ImageInferenceRequest(images, True)

        options = {
            "model": model_fixture,
            "parameters": parameters_fixture,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(benchmark, "xmodel", self.rest_client._infer, request, **options)
