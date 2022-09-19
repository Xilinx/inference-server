# Copyright 2022 Xilinx, Inc.
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

from .lib64._proteus.predict_api import *


def get_data(request_input: InferenceRequestInput):
    from _proteus import DataType

    datatype = request_input.datatype
    if datatype == DataType.BOOL:
        raise NotImplementedError("Bool datatype not supported")
    if datatype == DataType.UINT8:
        return request_input.getUint8Data()
    if datatype == DataType.UINT16:
        return request_input.getUint16Data()
    if datatype == DataType.UINT32:
        return request_input.getUint32Data()
    if datatype == DataType.UINT64:
        return request_input.getUint64Data()
    if datatype == DataType.INT8:
        return request_input.getInt8Data()
    if datatype == DataType.INT16:
        return request_input.getInt16Data()
    if datatype == DataType.INT32:
        return request_input.getInt32Data()
    if datatype == DataType.INT64:
        return request_input.getInt64Data()
    if datatype == DataType.FP16:
        raise NotImplementedError("FP16 datatype not supported")
    if datatype == DataType.FP32:
        return request_input.getFp32Data()
    if datatype == DataType.FP64:
        return request_input.getFp64Data()
    if datatype == DataType.STRING:
        return request_input.getStringData()
    raise NotImplementedError(f"{datatype.str()} not supported")


def to_dict(request: InferenceRequest):
    import numpy as np

    req = {}
    if request.id:
        req["id"] = request.id
    if request.parameters:
        for key, value in request.parameters:
            req[key] = value
    req["inputs"] = []
    inputs = request.getInputs()
    for inp in inputs:
        new_input = {}
        new_input["name"] = inp.name
        new_input["shape"] = inp.shape
        new_input["datatype"] = inp.datatype.str()
        if inp.parameters:
            for key, value in inp.parameters:
                new_input[key] = value
        data = get_data(inp)
        if isinstance(data, np.ndarray):
            data = data.tolist()
        new_input["data"] = data
        req["inputs"].append(new_input)
    outputs = request.getOutputs()
    if outputs:
        req["outputs"] = []
        for output in outputs:
            new_output = {}
            new_output["name"] = output.name
            if output.parameters:
                for key, value in output.parameters:
                    new_output[key] = value
            req["outputs"].append(new_output)
    return req
