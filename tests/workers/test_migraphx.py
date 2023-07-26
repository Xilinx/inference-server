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

import amdinfer
import amdinfer.pre_post as pre_post
import amdinfer.testing

from helper import root_path, run_benchmark


def preprocess_fp32(paths):
    """
    Given a list of paths to images, preprocess the images and return them

    Args:
        paths (list[str]): Paths to images

    Returns:
        list[numpy.ndarray]: List of images
    """

    options = pre_post.ImagePreprocessOptionsFloat()
    options.order = pre_post.ImageOrder.NCHW
    options.height = 224
    options.width = 224
    options.mean = [0.485, 0.456, 0.406]
    options.std = [4.367, 4.464, 4.444]
    options.normalize = True
    options.convert_color = True
    options.color_code = cv2.COLOR_BGR2RGB
    options.convert_type = True
    options.type = cv2.CV_32FC3
    options.convert_scale = 1.0 / 255.0
    return pre_post.imagePreprocessFp32(paths, options)


def preprocess_fp16(paths):
    """
    Given a list of paths to images, preprocess the images and return them

    Args:
        paths (list[str]): Paths to images

    Returns:
        list[numpy.ndarray]: List of images
    """

    options = pre_post.ImagePreprocessOptionsFp16()
    options.order = pre_post.ImageOrder.NCHW
    options.height = 224
    options.width = 224
    options.mean = np.array([0.485, 0.456, 0.406], np.float16)
    options.std = np.array([4.367, 4.464, 4.444], np.float16)
    options.normalize = True
    options.convert_color = True
    options.color_code = cv2.COLOR_BGR2RGB
    options.convert_type = True
    # in C++, CV_16F is defined to be 7. This constant doesn't exist in Python
    options.type = 7
    options.convert_scale = 1.0 / 255.0
    return pre_post.imagePreprocessFp16(paths, options)


def check_response(response, datatype):
    """
    Assert common checks on the response

    Args:
        response (InferenceResponse): response from the server
    """

    assert not response.isError(), response.getError()
    assert response.id == ""
    assert response.model == "migraphx"
    outputs = response.getOutputs()
    assert len(outputs) == 1
    for index, output in enumerate(outputs):
        assert output.name == ""
        assert output.datatype == datatype
        assert output.parameters.empty()


@pytest.mark.extensions(["migraphx"])
@pytest.mark.usefixtures("load")
class TestMigraphx:
    """
    Test the Migraphx worker
    """

    @staticmethod
    def get_config():
        model = "Migraphx"
        parameters = amdinfer.ParameterMap(
            ["model"], [amdinfer.testing.getPathToAsset("onnx_resnet50")]
        )
        return (model, parameters)

    @pytest.mark.parametrize("num", [1])
    def test_migraphx(self, num):
        """
        Send a request to model as tensor data
        """
        image_paths = [
            amdinfer.testing.getPathToAsset("asset_dog-3619020_640.jpg"),
        ]
        gold_responses = [[259, 261, 157, 260, 154]]
        assert len(image_paths) == len(gold_responses)
        image_num = len(image_paths)

        request_num = num
        for i in range(request_num):
            img = preprocess_fp32([image_paths[i % image_num]])[0]
            modelMetadata = None
            request = amdinfer.ImageInferenceRequest(img, modelMetadata, True)
            try:
                response = self.rest_client.modelInfer(self.endpoint, request)
            except ConnectionError:
                pytest.fail(
                    "Connection to the amdinfer server ended without response!", False
                )
            check_response(response, amdinfer.DataType.FP32)
            outputs = response.getOutputs()
            assert len(outputs) == 1

            top_k_responses = pre_post.resnet50PostprocessFp32(outputs[0], 5)

            assert top_k_responses == gold_responses[i % image_num]


@pytest.mark.extensions(["migraphx"])
@pytest.mark.usefixtures("load")
class TestMigraphxFp16:
    """
    Test the Migraphx worker with fp16 model
    """

    @staticmethod
    def get_config():
        model = "Migraphx"
        parameters = amdinfer.ParameterMap(
            ["model"], [amdinfer.testing.getPathToAsset("onnx_resnet50_fp16")]
        )
        return (model, parameters)

    @pytest.mark.parametrize("num", [1, 4])
    def test_migraphx_fp16(self, num):
        """
        Send a request to model as tensor data
        """
        image_paths = [
            amdinfer.testing.getPathToAsset("asset_dog-3619020_640.jpg"),
        ]
        gold_responses = [[259, 261, 260, 154, 157]]
        assert len(image_paths) == len(gold_responses)
        image_num = len(image_paths)

        request_num = num
        for i in range(request_num):
            img = preprocess_fp16([image_paths[i % image_num]])[0]
            modelMetadata = None
            request = amdinfer.ImageInferenceRequest(img, modelMetadata, True)
            try:
                response = self.rest_client.modelInfer(self.endpoint, request)
            except ConnectionError:
                pytest.fail(
                    "Connection to the amdinfer server ended without response!", False
                )
            check_response(response, amdinfer.DataType.FP16)
            outputs = response.getOutputs()
            assert len(outputs) == 1
            top_k_responses = pre_post.resnet50PostprocessFp16(outputs[0], 5)

            assert top_k_responses == gold_responses[i % image_num]

    @pytest.mark.parametrize("num", [1])
    def test_migraphx_fp16_fp32_data(self, num):
        """
        Sending FP32 data to a FP16 model should result in a graceful error

        Args:
            num (int): number of images
        """
        image_paths = [
            amdinfer.testing.getPathToAsset("asset_dog-3619020_640.jpg"),
        ]
        gold_responses = [[259, 261, 260, 154, 157]]
        assert len(image_paths) == len(gold_responses)
        image_num = len(image_paths)

        request_num = num
        for i in range(request_num):
            img = preprocess_fp32([image_paths[i % image_num]])[0]
            modelMetadata = None
            request = amdinfer.ImageInferenceRequest(img, modelMetadata, True)
            with pytest.raises(
                amdinfer.BadStatus,
                match='{"error":"Migraphx inference error: Migraph worker model and input data types don\'t match:   FP16 vs FP32"}',
            ):
                self.rest_client.modelInfer(self.endpoint, request)
