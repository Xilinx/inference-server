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

"""
This example shows how to use the Python API to start the server, make a request
to it over REST and parse the response. Look at the documentation for more
detailed commentary on this example.
"""

# fmt: off
# +imports:
from time import sleep

import numpy as np

import proteus

# -imports:
# fmt: on


def NumericalInferenceRequest(data, datatype=proteus.DataType.UINT32):
    if not isinstance(data, list):
        data = [data]
    request = proteus.predict_api.InferenceRequest()
    for index, datum in enumerate(data):
        input_0 = proteus.predict_api.InferenceRequestInput()
        input_0.name = f"input{index}"
        input_0.setUint32Data(np.array([datum], np.uint32))
        input_0.datatype = datatype
        input_0.shape = [1]
        request.addInputTensor(input_0)
    return request


# +main:
def main():
    # +create objects:
    client = proteus.clients.HttpClient("http://127.0.0.1:8998")
    # -create objects:

    # +start server: if it's not already started, start it from Python
    start_server = not client.serverLive()
    if start_server:
        server = proteus.servers.Server()
        server.startHttp(8998)
        while not client.serverLive():
            sleep(1)
    # -start server:

    # +load worker: load the Echo worker which accepts a number, adds 1, and returns the sum
    worker_name = client.workerLoad("Echo")

    ready = False
    while not ready:
        ready = client.modelReady(worker_name)
    # -load worker:

    # +inference: construct the request and make the inference
    data = [3]
    request = NumericalInferenceRequest(data)
    response = client.modelInfer(worker_name, request)
    # -inference:

    # +validate: check whether the inference succeeded by checking the response
    assert not response.isError(), response.getError()
    outputs = response.getOutputs()
    assert len(outputs) == len(data)
    for index, output in enumerate(outputs):
        recv_data = output.getUint32Data()
        assert len(recv_data) == 1
        assert recv_data[0] == data[index] + 1
    # -validate:

    print("hello_world_rest.py: Passed")


# -main:

if __name__ == "__main__":
    main()
