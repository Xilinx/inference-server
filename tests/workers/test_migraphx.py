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

import os
import sys

import pytest
import numpy as np
import proteus
import cv2

from helper import run_benchmark, root_path

sys.path.insert(0, os.path.join(root_path, "examples/python"))
from utils.utils import preprocess, postprocess


# The make_nxn and preprocess functions are based on an migraphx example at
# AMDMIGraphx/examples/vision/python_resnet50/resnet50_inference.ipynb
# The mean and standard dev. values used for this normalization are requirements of the Resnet50 model.

""" crop an image to square and then resize to desired dimension n x n"""


def make_nxn(image, n):
    width = image.shape[1]
    height = image.shape[0]
    if height > width:
        dif = height - width
        bar = dif // 2
        square = image[(bar + (dif % 2)) : (height - bar), :]
        return cv2.resize(square, (n, n))
    elif width > height:
        dif = width - height
        bar = dif // 2
        square = image[:, (bar + (dif % 2)) : (width - bar)]
        return cv2.resize(square, (n, n))
    else:
        return cv2.resize(image, (n, n))


"""# Normalize array values to the data type, mean and std. dev. required by Resnet50
# img_data: numpy array in 3 dimensions [channels, rows, cols] with value range 0-255"""


def preprocess(img_data):
    mean_vec = np.array([0.485, 0.456, 0.406])
    stddev_vec = np.array([0.229, 0.224, 0.225])
    norm_img_data = np.zeros(img_data.shape).astype("float32")
    for i in range(img_data.shape[0]):
        norm_img_data[i, :, :] = (img_data[i, :, :] / 255 - mean_vec[i]) / stddev_vec[i]
    return norm_img_data


@pytest.fixture(scope="class")
def model_fixture():
    return "Migraphx"


@pytest.fixture(scope="class")
def parameters_fixture():
    return {
        "model": str(
            root_path / "external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx"
        )
    }


# @pytest.mark.extensions(["migraphx"])
@pytest.mark.usefixtures("load")
class TestMigraphx:
    """
    Test the Migraphx worker
    """

    def send_request(self, request, check_asserts=True):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (InferenceRequest): request to send to the server
            output (np.ndarray): Output to check against golden output
            check_asserts (bool): Verify image against golden

        Returns:
            Response: Response as a dictionary
        """

        try:
            response = self.rest_client.modelInfer(self.model, request)
        except ConnectionError:
            pytest.fail(
                "Connection to the proteus server ended without response!", False
            )

        num_inputs = len(request.getInputs())

        if check_asserts:
            assert not response.isError(), response.getError()
            assert response.id == ""
            assert response.model == "migraphx"
            outputs = response.getOutputs()
            assert len(outputs) == num_inputs
            for index, output in enumerate(outputs):
                assert output.name == "input" + str(index)
                assert output.datatype == proteus.DataType.FP32
                assert output.parameters.empty()
        return response

    def test_Migraphx_0(self):
        """
        Send a request to model as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {"input_size": 224, "model": self.model}

        batch = 1
        images = []
        for _ in range(batch):
            # Load a picture
            img = cv2.imread(image_path).astype("float32")
            # Crop to a square, resize
            img = make_nxn(img, 224)
            #  Normalize contents with values specific to Resnet50
            img = img.transpose(2, 0, 1)
            img = preprocess(img)
            images.append(img)
        request = proteus.ImageInferenceRequest(images, True)
        response = self.send_request(request)
        k = postprocess(response, 5)
        # Look for the top 5 categories (not underlying scores)
        gold_response_output = [259, 261, 283, 230, 157]
        assert (k == gold_response_output).all()

    def test_Migraphx_1(self):
        """
        Send a request to model as tensor data
        """
        image_path = str(root_path / "tests/assets/dog-3619020_640.jpg")

        preprocessing = {"input_size": 224, "resize_method": "crop"}

        batch = 8
        images = []
        for _ in range(batch):
            # Load a picture
            img = cv2.imread(image_path).astype("float32")
            # Crop to a square, resize
            img = make_nxn(img, 224)
            #  Normalize contents with values specific to Resnet50
            img = img.transpose(2, 0, 1)
            img = preprocess(img)
            images.append(img)
        request = proteus.ImageInferenceRequest(images, True)
        response = self.send_request(request)
        k = postprocess(response, 5)
        gold_response_output = [259, 261, 283, 230, 157]
        assert (k == gold_response_output).all()

    @pytest.mark.benchmark(group="Migraphx")
    def test_benchmark_Migraphx(self, benchmark, model_fixture, parameters_fixture):

        batch_size = 16
        input_size = parameters_fixture.get("input_size")
        images = np.random.uniform(
            0.0, 255.0, (batch_size, input_size, input_size, 3)
        ).astype(np.float32)
        images = [image for image in images]

        request = proteus.ImageInferenceRequest(images, True)

        options = {
            "model": model_fixture,
            "parameters": parameters_fixture,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "Migraphx", self.rest_client.modelInfer, request, **options
        )
