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

import pytest

import amdinfer
import amdinfer.pre_post as pre_post
import amdinfer.testing

from helper import run_benchmark


def preprocess(paths):
    options = pre_post.ImagePreprocessOptionsInt8()
    options.order = pre_post.ImageOrder.NHWC
    options.mean = [123, 107, 104]
    options.std = [1, 1, 1]
    options.normalize = True
    return pre_post.imagePreprocessInt8(paths, options)


def postprocess(output, k):
    return pre_post.resnet50PostprocessInt8(output, k)


def validate(responses, golden):
    k = len(golden)
    for response in responses:
        assert not response.isError(), response.getError()
        assert response.id == ""
        assert response.model == "xmodel"
        outputs = response.getOutputs()
        assert len(outputs) == 1
        for index, output in enumerate(outputs):
            assert output.name == ""
            assert output.datatype == amdinfer.DataType.INT8
            assert output.parameters.empty()
            top_categories = postprocess(output, k)
            assert top_categories == golden


@pytest.mark.extensions(["vitis"])
@pytest.mark.fpgas("DPUCADF8H", 1)
@pytest.mark.usefixtures("load")
class TestXmodelU250:
    """
    Test the Xmodel worker on u250 board
    """

    @staticmethod
    def get_config():
        model = "Xmodel"
        parameters = amdinfer.ParameterMap(
            ["model"], [amdinfer.testing.getPathToAsset("u250_resnet50")]
        )
        return (model, parameters)

    def test_xmodel_0(self):
        """
        Send a request to resnet50 as tensor data
        """
        image_path = amdinfer.testing.getPathToAsset("asset_dog-3619020_640.jpg")

        images = preprocess([image_path])
        request = amdinfer.ImageInferenceRequest(
            images, self.rest_client.modelMetadata(self.endpoint), True
        )
        response = self.rest_client.modelInfer(self.endpoint, request)
        golden = [259, 261, 260, 157, 230]
        validate([response], golden)

    @pytest.mark.benchmark(group="xmodel")
    def test_benchmark_xmodel(self, benchmark):
        image_path = amdinfer.testing.getPathToAsset("asset_dog-3619020_640.jpg")

        images = preprocess([image_path])
        request = amdinfer.ImageInferenceRequest(
            images, self.rest_client.modelMetadata(self.endpoint), True
        )

        options = {
            "model": self.model,
            "parameters": self.parameters,
            "type": "rest (pytest)",
            "config": "N/A",
        }
        run_benchmark(
            benchmark, "xmodel", self.rest_client.modelInfer, request, **options
        )


@pytest.mark.extensions(["vitis"])
@pytest.mark.fpgas("DPUCVDX8H", 1)
@pytest.mark.usefixtures("load")
@pytest.mark.parametrize(
    "server_environment",
    [{"XLNX_VART_FIRMWARE": "/opt/xilinx/overlaybins/DPUCVDX8H/4PE/"}],
)
class TestXmodelVck50004pe:
    """
    Test the Xmodel worker on vck5000-4pe board
    """

    @staticmethod
    def get_config():
        model = "Xmodel"
        parameters = amdinfer.ParameterMap(
            ["model"], [amdinfer.testing.getPathToAsset("vck5000-4pe_resnet50")]
        )
        return (model, parameters)

    def test_xmodel_0(self):
        """
        Send a request to resnet50 as tensor data
        """
        image_path = amdinfer.testing.getPathToAsset("asset_dog-3619020_640.jpg")
        images = preprocess([image_path])
        request = amdinfer.ImageInferenceRequest(
            images, self.rest_client.modelMetadata(self.endpoint), True
        )
        response = self.rest_client.modelInfer(self.endpoint, request)
        golden = [259, 261, 260, 157, 154]
        validate([response], golden)
