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
import pathlib
import os
import re

import cv2
import base64
import numpy as np

import proteus
from proteus.predict_api import InferenceRequest, InferenceRequestInput

root = os.getenv("PROTEUS_ROOT")
if root is None:
    raise RuntimeError("PROTEUS_ROOT not defined in the environment")
root_path = pathlib.Path(root)
try:
    with open(root_path / "build/config.txt", "r") as f:
        build = f.read().strip().split(" ")[0]
except FileNotFoundError:
    print("No config.txt found in build/. Using default value of 'Debug'")
    build = "Debug"
build_path = root_path / f"build/{build}"
run_path = build_path / "src/proteus/proteus-server"


def getConstexprFromHeader(constexpr, file):
    with open(file) as f:
        for line in f:
            if line.startswith(f"constexpr auto {constexpr}"):
                matches = re.match(r".* = (.*);", line)
                if matches:
                    return matches.group(1)


def write_lua(filename: str, data: str):
    lua_file = [
        "done = function(summary, latency, requests)",
        '  io.write("------------------------------\\n")',
        '  io.write(string.format("%u,%u,%f,%f\\n", latency.min, latency.max, latency.mean, latency.stdev))',
        '  io.write(string.format("%u,%u,%f,%f\\n", requests.min, requests.max, requests.mean, requests.stdev))',
        '  io.write(string.format("%u,%u,%u\\n", summary.duration, summary.requests, summary.bytes))',
        "end",
        'wrk.method = "POST"',
        'wrk.headers["Content-Type"] = "application/json"',
        f"wrk.body = '{data}'",
    ]

    with open(root_path / "tests/workers" / (filename + ".lua"), "w") as f:
        for line in lua_file:
            f.write(line + "\n")


def run_benchmark_func(benchmark, func, **kwargs):
    benchmark.extra_info["model"] = kwargs["model"]
    for key, value in kwargs.items():
        benchmark.extra_info[key] = value
    benchmark(func)


def run_benchmark(benchmark, benchmark_name, func, request, **kwargs):
    write_lua(benchmark_name, json.dumps(request.asdict()))
    benchmark.extra_info["lua"] = benchmark_name
    benchmark.extra_info["model"] = kwargs["model"]
    for key, value in kwargs.items():
        benchmark.extra_info[key] = value
    benchmark(func, kwargs["model"], request)


kDefaultHttpPort = int(
    getConstexprFromHeader(
        "kDefaultHttpPort", root_path / "include/proteus/build_options.hpp"
    )
)


def NumericalInferenceRequest(data, datatype=proteus.DataType.INT32):
    if not isinstance(data, list):
        data = [data]
    request = InferenceRequest()
    for index, datum in enumerate(data):
        input = InferenceRequestInput(f"input{index}")
        input.data = [datum]
        input.datatype = datatype
        input.shape = [1]
        request.addInputTensor(input)
    return request


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
            # if asTensor:
            read_image = cv2.imread(image)
            input_n.datatype = getattr(proteus.DataType, str(read_image.dtype).upper())
            input_n.shape = [*read_image.shape]  # Convert tuple to list
            input_n = set_data(input_n, read_image.flatten())
            # else:
            #     input_n.datatype = proteus.DataType.STRING
            #     with open(image, "rb") as f:
            #         input_n.data = [base64.b64encode(f.read()).decode("utf-8")]
            #     input_n.shape = [len(input.data[0])]
        elif isinstance(image, np.ndarray):
            input_n.datatype = getattr(proteus.DataType, str(image.dtype).upper())
            input_n.shape = [*image.shape]  # Convert tuple to list
            set_data(input_n, image.flatten())
        else:
            raise TypeError("Unknown type passed to ImageInferenceRequest")
        request.addInputTensor(input_n)
    return request
