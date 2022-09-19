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

import functools
import multiprocessing as mp
import sys
import time

from .lib64._proteus import *


def _set_data(input_n, image):
    if input_n.datatype == DataType.UINT8:
        input_n.setUint8Data(image)
    elif input_n.datatype == DataType.UINT16:
        input_n.setUint16Data(image)
    elif input_n.datatype == DataType.UINT32:
        input_n.setUint32Data(image)
    elif input_n.datatype == DataType.UINT64:
        input_n.setUint64Data(image)
    elif input_n.datatype == DataType.INT8:
        input_n.setInt8Data(image)
    elif input_n.datatype == DataType.INT16:
        input_n.setInt16Data(image)
    elif input_n.datatype == DataType.INT32:
        input_n.setInt32Data(image)
    elif input_n.datatype == DataType.INT64:
        input_n.setInt64Data(image)
    elif input_n.datatype == DataType.FP32:
        input_n.setFp32Data(image)
    elif input_n.datatype == DataType.FP64:
        input_n.setFp64Data(image)
    elif input_n.datatype == DataType.STRING:
        input_n.setStringData(image)
    else:
        raise ValueError("Unsupported type")
    return input_n


def ImageInferenceRequest(images, asTensor=False):
    """
    Construct a request from an image or list of images

    Args:
        images (image): Images may be numpy arrays or filepaths or a list of these
        asTensor (bool, optional): Send data as a tensor or as base64-encoded string. Defaults to False.

    Raises:
        TypeError: Raised if an unknown image format is passed

    Returns:
        InferenceRequest: Request object
    """
    import base64

    import cv2
    import numpy as np

    if not isinstance(images, list):
        images = [images]
    request = predict_api.InferenceRequest()
    for index, image in enumerate(images):
        input_n = predict_api.InferenceRequestInput()
        input_n.name = f"input{index}"
        if isinstance(image, str):
            if asTensor:
                read_image = cv2.imread(image)
                input_n.datatype = getattr(DataType, str(read_image.dtype).upper())
                input_n.shape = [*read_image.shape]  # Convert tuple to list
                input_n = _set_data(input_n, read_image.flatten())
            else:
                input_n.datatype = DataType.STRING
                with open(image, "rb") as f:
                    data = base64.b64encode(f.read()).decode("utf-8")
                    input_n = _set_data(input_n, data)
                    input_n.shape = [len(data)]
        elif isinstance(image, np.ndarray):
            input_n.datatype = getattr(DataType, str(image.dtype).upper())
            input_n.shape = [*image.shape]  # Convert tuple to list
            _set_data(input_n, image.flatten())
        else:
            raise TypeError("Unknown type passed to ImageInferenceRequest")
        request.addInputTensor(input_n)
    return request


def _infer(client, model, image):
    """
    Make an inference. For multiprocessing, the function must be defined at the
    top level

    Args:
        client (proteus.client): client to send request with
        model (str): name of the model/worker to make the inference
        image (np.array): Image to send to the server
    """
    request = ImageInferenceRequest(image)
    response = client.modelInfer(model, request)
    assert not response.isError(), response.getError()


def parallel_infer(client, model, data, processes):
    """
    Make an inference to the server in parallel with n processes

    Args:
        client (proteus.client): Client to make the inference with
        model (str): Name of the model/worker to make the inference
        data (list[np.ndarray]): List of data to send
        processes (int): number of processes to use

    Returns:
        list[proteus.InferenceResponse]: Responses for each request
    """

    make_inference = functools.partial(_infer, client, model)
    with mp.Pool(processes) as p:
        p.map(make_inference, data)
        p.close()
        p.join()


def start_http_client_server(address: str, extension=None):
    client = clients.HttpClient(address)
    port = address.split(":")[-1]

    # if it's not already started, start it here
    start_server = not client.serverLive()
    if start_server:
        server = servers.Server()
        server.startHttp(int(port))
        while not client.serverLive():
            time.sleep(1)
    else:
        server = None

    if extension:
        metadata = client.serverMetadata()
        if extension not in metadata.extensions:
            print(f"{extension} support required but not found.")
            sys.exit(0)

    return client, server
