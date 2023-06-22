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

import argparse
import base64
import json
import os

import cv2
import numpy as np
import pytest

import amdinfer
import amdinfer.testing

from helper import root_path, run_benchmark, run_benchmark_func


def compare_jpgs(test_image, reference_image):
    """
    Compare a base64 encoded JPG image string with a reference image (as a np
    buffer). Since JPGs are lossy, images that are "equal" may be slightly
    different. We use the Manhattan norm to estimate the relative difference
    between the images.

    Args:
        test_image (str): Base64-encoded string without the header
        reference_image (np.ndarray): The image to compare against
        shape (list, optional): Check that the labeled shape of the test image
            matches its actual decoded shape. Defaults to None.
    """
    decoded_image = base64.decodebytes(test_image)
    buffer = np.frombuffer(decoded_image, dtype=np.uint8)
    received_image = cv2.imdecode(buffer, cv2.IMREAD_UNCHANGED)
    diff = received_image.astype(np.int16) - reference_image.astype(np.int16)
    m_norm = np.sum(np.absolute(diff))  # Manhattan norm
    m_norm_per_pixel = m_norm / np.prod(reference_image.shape)
    assert m_norm_per_pixel < 2  # arbitrarily use a threshold of 2 for comparison


def construct_request(asTensor):
    image_path = amdinfer.testing.getPathToAsset("asset_dog-3619020_640.jpg")
    modelMetadata = None
    request = amdinfer.ImageInferenceRequest(image_path, modelMetadata, asTensor)

    image = cv2.imread(image_path)
    image = cv2.bitwise_not(image)

    return request, image


def validate(response, image):
    assert not response.isError(), response.getError()

    assert response.model == "invert_image"

    outputs = response.getOutputs()
    assert len(outputs) == 1

    output = outputs[0]
    if output.datatype == amdinfer.DataType.BYTES:
        data = output.getStringData()
        compare_jpgs(data, image)
        assert output.parameters.empty()
    else:
        assert output.shape == [*image.shape]
        assert output.datatype == amdinfer.DataType.UINT8
        data = output.getUint8Data()
        assert len(data) == image.size
        assert (data == image.flatten().tolist()).all()
        assert output.parameters.empty()

    return response


@pytest.mark.usefixtures("load")
class TestInvertImage:
    """
    Test the InvertImage worker
    """

    @staticmethod
    def get_config():
        model = "CPlusPlus"
        parameters = amdinfer.ParameterMap()
        parameters.put("model", "invert_image")
        return (model, parameters)

    def send_request(self, request, image):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (dict): request to send to the server
            image (np.ndarray): Golden image to check against
            check_asserts (bool): Verify image against golden

        Returns:
            Response: Response
        """

        try:
            response = self.rest_client.modelInfer(self.endpoint, request)
        except ConnectionError:
            pytest.fail(
                "Connection to the amdinfer server ended without response!", False
            )

        validate(response, image)

    def test_invert_image(self):
        """
        Send a request to InvertImage as tensor data
        """
        request, image = construct_request(True)
        self.send_request(request, image)

    @pytest.mark.benchmark(group="invert_image")
    def test_benchmark_invert_image(self, benchmark):
        options = {
            "model": self.model,
            "parameters": self.parameters,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark_func(benchmark, self.test_invert_image, **options)

    @pytest.mark.benchmark(group="invert_image")
    def test_benchmark_invert_image_request(self, benchmark):
        request, _ = self.construct_request(True)
        options = {
            "model": self.model,
            "parameters": self.parameters,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "invert_image_0", self.rest_client.modelInfer, request, **options
        )


@pytest.mark.usefixtures("load")
class TestInvertImage2:
    """
    Test the InvertImage worker
    """

    @staticmethod
    def get_config():
        model = ["CPlusPlus", "CPlusPlus", "CPlusPlus"]
        parameters_0 = amdinfer.ParameterMap()
        parameters_0.put("model", "base64_decode")
        parameters_1 = amdinfer.ParameterMap()
        parameters_1.put("model", "invert_image")
        parameters_2 = amdinfer.ParameterMap()
        parameters_2.put("model", "base64_encode")
        return (model, [parameters_0, parameters_1, parameters_2])

    def send_request(self, request, image):
        """
        Sends the given request to the server and asserts common checks

        Args:
            request (dict): request to send to the server
            image (np.ndarray): Golden image to check against
            check_asserts (bool): Verify image against golden

        Returns:
            Response: Response
        """

        try:
            response = self.rest_client.modelInfer(self.endpoint, request)
        except ConnectionError:
            pytest.fail(
                "Connection to the amdinfer server ended without response!", False
            )

        validate(response, image)

    def test_invert_image(self):
        """
        Send a request to InvertImage as a base64-encoded image
        """

        request, image = construct_request(False)
        self.send_request(request, image)

    @pytest.mark.benchmark(group="invert_image")
    def test_benchmark_invert_image(self, benchmark):
        options = {
            "model": self.model,
            "parameters": self.parameters,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark_func(benchmark, self.test_invert_image, **options)

    @pytest.mark.benchmark(group="invert_image")
    def test_benchmark_invert_image_request(self, benchmark):
        request, _ = self.construct_request(False)
        options = {
            "model": self.model,
            "parameters": self.parameters,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "invert_image_1", self.rest_client.modelInfer, request, **options
        )
