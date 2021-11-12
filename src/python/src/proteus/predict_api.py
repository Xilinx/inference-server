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

from enum import Enum

import base64
import cv2
import numpy as np


class Datatype(Enum):
    BOOL = "BOOL"
    UINT8 = "UINT8"
    UINT16 = "UINT16"
    UINT32 = "UINT32"
    UINT64 = "UINT64"
    INT8 = "INT8"
    INT16 = "INT16"
    INT32 = "INT32"
    INT64 = "INT64"
    FP16 = "FP16"
    FP32 = "FP32"
    FP64 = "FP64"
    STRING = "STRING"


class RequestInput:
    def __init__(self, name: str):
        """
        Construct a RequestInput object

        Args:
            name (str): Name of the input
        """
        self.name = name
        self.shape = []
        self.datatype: Datatype = Datatype.UINT32
        self.parameters = {}
        self.data = []

    def asdict(self):
        """
        Return the object as a dictionary

        Returns:
            dict: Dictionary representing the object
        """
        tmp = {}
        tmp["name"] = self.name
        tmp["shape"] = self.shape
        tmp["datatype"] = self.datatype.value
        if self.parameters:
            tmp["parameters"] = self.parameters
        tmp["data"] = self.data
        return tmp


class RequestOutput:
    def __init__(self, name):
        """
        Construct a RequestOutput object

        Args:
            name (str): Name of the output
        """
        self.name = name
        self.parameters = {}

    def asdict(self):
        """
        Return the object as a dictionary

        Returns:
            dict: Dictionary representing the object
        """
        tmp = {}
        tmp["name"] = self.name
        if self.parameters:
            tmp["parameters"] = self.parameters
        return tmp


class InferenceRequest:
    def __init__(self, inputs=None, id=None, parameters=None, outputs=None):
        if id:
            self.id = id
        else:
            self.id = ""

        if parameters:
            self.parameters = parameters
        else:
            self.parameters = {}

        if inputs:
            if not isinstance(inputs, list):
                inputs = [inputs]
            self.inputs = inputs
        else:
            self.inputs = []

        if outputs:
            self.outputs = outputs
        else:
            self.outputs = []

    def asdict(self):
        tmp = {}
        if self.id:
            tmp["id"] = self.id
        if self.parameters:
            tmp["parameters"] = self.parameters
        tmp["inputs"] = []
        if isinstance(self.inputs, list):
            for input in self.inputs:
                tmp["inputs"].append(input.asdict())
        else:
            tmp["inputs"].append(self.inputs.asdict())
        if self.outputs:
            tmp["outputs"] = []
            for output in self.outputs:
                tmp["outputs"].append(output.asdict())
        return tmp


class NumericalInferenceRequest(InferenceRequest):
    def __init__(self, data, datatype=Datatype.INT32):
        if not isinstance(data, list):
            data = [data]
        inputs = []
        for index, datum in enumerate(data):
            input = RequestInput(f"input{index}")
            input.data = [datum]
            input.datatype = datatype
            input.shape = [1]
            inputs.append(input)
        super().__init__(inputs)


class ImageInferenceRequest(InferenceRequest):
    def __init__(self, images, asTensor=False):
        if not isinstance(images, list):
            images = [images]
        inputs = []
        for index, image in enumerate(images):
            input = RequestInput(f"input{index}")
            if isinstance(image, str):
                if asTensor:
                    read_image = cv2.imread(image)
                    input.datatype = Datatype(str(read_image.dtype).upper())
                    input.shape = [*read_image.shape]  # Convert tuple to list
                    input.data = read_image.flatten().tolist()
                else:
                    input.datatype = Datatype.STRING
                    with open(image, "rb") as f:
                        input.data = [base64.b64encode(f.read()).decode("utf-8")]
                    input.shape = [len(input.data[0])]
            elif isinstance(image, np.ndarray):
                input.datatype = Datatype(str(image.dtype).upper())
                input.shape = [*image.shape]  # Convert tuple to list
                input.data = image.flatten().tolist()
            else:
                raise TypeError("Unknown type passed to ImageInferenceRequest")

            inputs.append(input)
        super().__init__(inputs)


class WebsocketInferenceRequest(InferenceRequest):
    def __init__(self, model, inputs=None, id=None, parameters=None, outputs=None):
        self.model = model
        super().__init__(inputs, id, parameters, outputs)

    def asdict(self):
        tmp = super().asdict()
        tmp["model"] = self.model
        return tmp


class Response:
    def __init__(self, error):
        self.error = error


class InferenceResponse(Response):
    def __init__(self, response):
        super().__init__(False)
        if "model_version" in response:
            self.model_version = response["model_version"]
        else:
            self.model_version = ""

        if "parameters" in response:
            self.parameters = response["paramaters"]
        else:
            self.parameters = {}

        self.model_name = response["model_name"]
        self.id = response["id"]

        self.outputs = []
        for output in response["outputs"]:
            self.outputs.append(ResponseOutput(output))


class ResponseOutput:
    def __init__(self, output):
        self.name = output["name"]
        self.shape = output["shape"]
        self.datatype = Datatype(output["datatype"])
        if "parameters" in output:
            self.parameters = output["parameters"]
        else:
            self.parameters = {}
        self.data = output["data"]


class ErrorResponse(Response):
    def __init__(self, response):
        super().__init__(True)
        if "application/json" in response.headers.get("content-type"):
            content = response.json()
            self.error_msg = content["error"]
        else:
            content = response.content
            self.error_msg = response.content.decode("utf-8")
        self.status_code = response.status_code


class HtmlResponse(Response):
    def __init__(self, response):
        super().__init__(False)
        self.html = response
