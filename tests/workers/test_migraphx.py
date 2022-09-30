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

import cv2
import numpy as np
import pytest

import proteus
import proteus.testing

from helper import root_path, run_benchmark

sys.path.insert(0, os.path.join(root_path, "examples/python"))
from utils.utils import postprocess

# The make_nxn and preprocess functions are based on an migraphx example at
# AMDMIGraphx/examples/vision/python_resnet50/resnet50_inference.ipynb
# The mean and standard dev. values used for this normalization are requirements of the Resnet50 model.


def make_nxn(image, n):
    """
    Crop an image to square and then resize to desired dimension n x n
    """
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


def preprocess(img_data):
    """
    Normalize array values to the data type, mean and std. dev. required by Resnet50

    Args:
        img_data (np.array): array in 3 dimensions [channels, rows, cols] with value range 0-255
        The vectors are for RGB images, so images read with OpenCV must have channels
        converted before calling.
    Returns:
        Response: np.array
    """
    mean_vec = np.array([0.406, 0.456, 0.485])
    stddev_vec = np.array([0.225, 0.224, 0.229])
    norm_img_data = np.zeros(img_data.shape).astype("float32")
    for i in range(img_data.shape[0]):
        norm_img_data[i, :, :] = (img_data[i, :, :] / 255 - mean_vec[i]) / stddev_vec[i]
    return norm_img_data


@pytest.mark.extensions(["migraphx"])
@pytest.mark.usefixtures("load")
class TestMigraphx:
    """
    Test the Migraphx worker
    """

    model = "Migraphx"
    parameters = {"model": proteus.testing.getPathToAsset("onnx_resnet50")}

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
            response = self.rest_client.modelInfer(self.endpoint, request)
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

    @pytest.mark.parametrize("num", [1])
    def test_migraphx(self, num):
        """
        Send a request to model as tensor data
        """
        image_paths = [
            proteus.testing.getPathToAsset("asset_dog-3619020_640.jpg"),
        ]
        gold_responses = [[259, 261, 157, 260, 154]]
        assert len(image_paths) == len(gold_responses)
        image_num = len(image_paths)

        request_num = num
        for i in range(request_num):
            # Load a picture
            img = cv2.imread(image_paths[i % image_num]).astype("float32")
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            # Crop to a square, resize
            img = make_nxn(img, 224)
            #  Normalize contents with values specific to Resnet50
            img = img.transpose(2, 0, 1)
            img = preprocess(img)

            request = proteus.ImageInferenceRequest(img, True)
            response = self.send_request(request)
            top_k_responses = postprocess(response, 5)

            for i, top_k in enumerate(top_k_responses):
                # Look for the top 5 categories (not underlying scores)
                assert (top_k == gold_responses[i % image_num]).all()
