# Copyright 2022 Xilinx Inc.
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

import cv2
import base64
import numpy as np

import proteus
from proteus.predict_api import InferenceRequest, InferenceRequestInput

# TODO(varunsh): delete duplicate ImageInferenceRequest
def set_data(input_n, image):
    if input_n.datatype == proteus.DataType.UINT8:
        input_n.setUint8Data(image)
    elif input_n.datatype == proteus.DataType.UINT16:
        input_n.setUint16Data(image)
    elif input_n.datatype == proteus.DataType.UINT32:
        input_n.setUint32Data(image)
    elif input_n.datatype == proteus.DataType.UINT64:
        input_n.setUint64Data(image)
    elif input_n.datatype == proteus.DataType.INT8:
        input_n.setInt8Data(image)
    elif input_n.datatype == proteus.DataType.INT16:
        input_n.setInt16Data(image)
    elif input_n.datatype == proteus.DataType.INT32:
        input_n.setInt32Data(image)
    elif input_n.datatype == proteus.DataType.INT64:
        input_n.setInt64Data(image)
    elif input_n.datatype == proteus.DataType.FP32:
        input_n.setFp32Data(image)
    elif input_n.datatype == proteus.DataType.FP64:
        input_n.setFp64Data(image)
    elif input_n.datatype == proteus.DataType.STRING:
        input_n.setStringData(image)
    else:
        raise ValueError("Unsupported type")
    return input_n


def ImageInferenceRequest(images, asTensor=False):
    if not isinstance(images, list):
        images = [images]
    request = InferenceRequest()
    for index, image in enumerate(images):
        input_n = InferenceRequestInput()
        input_n.name = f"input{index}"
        if isinstance(image, str):
            if asTensor:
                read_image = cv2.imread(image)
                input_n.datatype = getattr(
                    proteus.DataType, str(read_image.dtype).upper()
                )
                input_n.shape = [*read_image.shape]  # Convert tuple to list
                input_n = set_data(input_n, read_image.flatten())
            else:
                input_n.datatype = proteus.DataType.STRING
                with open(image, "rb") as f:
                    data = base64.b64encode(f.read()).decode("utf-8")
                    input_n = set_data(input_n, data)
                    input_n.shape = [len(data)]
        elif isinstance(image, np.ndarray):
            input_n.datatype = getattr(proteus.DataType, str(image.dtype).upper())
            input_n.shape = [*image.shape]  # Convert tuple to list
            set_data(input_n, image.flatten())
        else:
            raise TypeError("Unknown type passed to ImageInferenceRequest")
        request.addInputTensor(input_n)
    return request
