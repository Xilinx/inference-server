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
to it over REST and parse the response. In contrast to the simpler Hello World
example, we make an inference to a custom XModel and show how to pre- and post-
process the results. Look at the documentation for more detailed commentary on
this example.
"""

import os
import pathlib
import sys
from time import sleep

import cv2

import proteus
import proteus.client_operators
import proteus.clients
import proteus.servers
import proteus.util.pre_post as pre_post

# isort: split

from resnet import parse_args, print_label, resolve_image_paths


def preprocess(paths):
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
    return pre_post.imagePreprocessFloat(paths, options)


def postprocess(output, k):
    return pre_post.resnet50PostprocessFloat(output, k)


def construct_requests(images):
    requests = []
    for image in images:
        requests.append(proteus.ImageInferenceRequest(image))
    return requests


def load(client, args):
    metadata = client.serverMetadata()

    if "migraphx" not in metadata.extensions:
        print(
            "MIGraphX is not enabled. Please recompile with it enabled to run this example"
        )
        sys.exit(1)

    parameters = proteus.RequestParameters()
    # batcher timeout value in milliseconds
    timout_ms = 1000
    parameters.put("model", args.model)
    parameters.put("timeout", timout_ms)
    endpoint = client.workerLoad("migraphx", parameters)
    return endpoint


def get_args():
    args = parse_args()

    if not args.model:
        root = os.getenv("PROTEUS_ROOT")
        args.model = root + "/external/artifacts/onnx/resnet50v2/resnet50-v2-7.onnx"

    return args


def main(args):
    client = proteus.clients.HttpClient(f"http://127.0.0.1:{args.http_port}")

    server = proteus.servers.Server()
    print("Waiting until the server is ready...")
    server.startHttp(args.http_port)

    ready = False
    while not ready:
        try:
            ready = client.serverReady()
        except proteus.RuntimeError:
            pass
        sleep(1)

    print("Loading worker...")
    endpoint = load(client, args)

    ready = False
    while not ready:
        ready = client.modelReady(endpoint)

    paths = resolve_image_paths(pathlib.Path(args.image))

    images = preprocess(paths)

    requests = construct_requests(images)

    assert len(paths) == len(requests)
    # +run inference
    # migraphx.py
    responses = proteus.client_operators.inferAsyncOrdered(client, endpoint, requests)
    print("Making inferences...")
    for image_path, response in zip(paths, responses):
        assert not response.isError()

        outputs = response.getOutputs()
        assert len(outputs) == 1
        top_indices = postprocess(outputs[0], args.top)
        print_label(top_indices, args.labels, image_path)
    # -run inference


if __name__ == "__main__":

    args = get_args()

    main(args)
