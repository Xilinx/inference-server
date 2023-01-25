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

"""
This example shows how to use the Python API to start the server, make a request
to it over REST and parse the response. Look at the documentation for more
detailed commentary on this example.
"""

# +imports
from time import sleep

import numpy as np

import amdinfer

# -imports


# +make request
def make_request(data):
    """
    Make a request containing an integer

    Args:
        data (int): Data to send

    Returns:
        amdinfer.InferenceRequest: Request
    """
    request = amdinfer.InferenceRequest()
    # each request has one or more input tensors, depending on the worker/model that's going to process it
    input_0 = amdinfer.InferenceRequestInput()
    input_0.name = f"input0"
    input_0.setUint32Data(np.array([data], np.uint32))
    input_0.datatype = amdinfer.DataType.UINT32
    input_0.shape = [1]
    request.addInputTensor(input_0)
    return request


# -make request


def main():
    # +create objects
    client = amdinfer.HttpClient("http://127.0.0.1:8998")
    # -create objects

    # +start server: if it's not already started, start it from Python
    start_server = not client.serverLive()
    if start_server:
        print("No server detected. Starting locally...")
        server = amdinfer.Server()
        server.startHttp(8998)
    amdinfer.waitUntilServerReady(client)
    # -start server:
    print("Server ready!")

    # +load worker: load the Echo worker which accepts a number, adds 1, and returns the sum
    endpoint = client.workerLoad("echo")
    amdinfer.waitUntilModelReady(client, endpoint)
    # -load worker
    print("Model ready!")

    # +inference: construct the request and make the inference
    data = 3
    request = make_request(data)
    response = client.modelInfer(endpoint, request)
    # -inference
    print("Made inference!")

    # +validate: check whether the inference succeeded by checking the response
    assert not response.isError(), response.getError()
    outputs = response.getOutputs()
    assert len(outputs) == 1
    for output in outputs:
        recv_data = output.getUint32Data()
        assert len(recv_data) == 1
        assert recv_data[0] == data + 1
    # -validate
    print("Results validated!")


if __name__ == "__main__":
    main()
