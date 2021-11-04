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

# +imports:
import proteus

# -imports:

# +main:
def main():
    # +create objects:
    server = proteus.Server()
    client = proteus.RestClient("127.0.0.1:8998")
    # -create objects:

    # +start server: if it's not already started, start it from Python
    try:
        start_server = not client.server_live()
    except proteus.ConnectionError:
        start_server = True
    if start_server:
        server.start(quiet=True)
        client.wait_until_live()
    # -start server:

    # +load worker: load the Echo worker which accepts a number, adds 1, and returns the sum
    response = client.load("Echo")
    assert not response.error, response.error_msg
    worker_name = response.html

    while not client.model_ready(worker_name):
        pass
    # -load worker:

    # +inference: construct the request and make the inference
    data = [3, 1, 4, 1, 5]
    request = proteus.NumericalInferenceRequest(data)
    response = client.infer(worker_name, request)
    # -inference:

    # +validate: check whether the inference succeeded by checking the response
    assert not response.error, response.error_msg
    assert len(response.outputs) == len(data)
    for index, output in enumerate(response.outputs):
        assert len(output.data) == 1
        assert output.data[0] == data[index] + 1
    # -validate:

    # +clean up: stop the server if it was started from Python
    if start_server:
        server.stop()
        client.wait_until_stop()
    # -clean up:

    print("Passed hello_world_rest.py")


# -main:

if __name__ == "__main__":
    main()
