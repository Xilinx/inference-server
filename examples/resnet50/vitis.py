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

# +import
import proteus
import proteus.clients
import proteus.servers
import proteus.util.pre_post as pre_post

# -import

# isort: split

from resnet import parse_args, print_label, resolve_image_paths


def preprocess(paths):
    options = pre_post.ImagePreprocessOptionsInt8()
    options.order = pre_post.ImageOrder.NHWC
    options.mean = [123, 107, 104]
    options.std = [1, 1, 1]
    options.normalize = True
    return pre_post.imagePreprocessInt8(paths, options)


def postprocess(output, k):
    return pre_post.resnet50PostprocessInt8(output, k)


# +construct request
def construct_requests(images):
    requests = []
    for image in images:
        requests.append(proteus.ImageInferenceRequest(image))
    return requests


# -construct request


def load(client, args):
    metadata = client.serverMetadata()

    if "vitis" not in metadata.extensions:
        print(
            "Vitis AI is not enabled. Please recompile with it enabled to run this example"
        )
        sys.exit(1)

    # +load
    parameters = proteus.RequestParameters()
    parameters.put("model", args.model)
    endpoint = client.workerLoad("xmodel", parameters)
    # -load
    return endpoint


def get_args():
    args = parse_args()

    if not args.model:
        root = os.getenv("PROTEUS_ROOT")
        args.model = (
            root
            + "/external/artifacts/u200_u250/resnet_v1_50_tf/resnet_v1_50_tf.xmodel"
        )

    return args


def main(args):
    # +initialize
    server = proteus.servers.Server()
    # -initialize
    print("Waiting until the server is ready...")
    # +start protocol
    server.startHttp(args.http_port)
    # -start protocol

    # + create client
    client = proteus.clients.HttpClient(f"http://127.0.0.1:{args.http_port}")
    # - create client
    ready = False
    while not ready:
        try:
            ready = client.serverReady()
        except proteus.RuntimeError:
            pass
        sleep(1)

    print("Loading worker...")
    endpoint = load(client, args)

    # +wait model ready
    ready = False
    while not ready:
        ready = client.modelReady(endpoint)
    # -wait model ready

    # +prepare images
    paths = resolve_image_paths(pathlib.Path(args.image))
    images = preprocess(paths)
    # -prepare images

    requests = construct_requests(images)

    assert len(paths) == len(requests)
    print("Making inferences...")
    # +run inference
    # vitis.py
    for image_path, request in zip(paths, requests):
        response = client.modelInfer(endpoint, request)
        assert not response.isError()

        outputs = response.getOutputs()
        assert len(outputs) == 1
        top_indices = postprocess(outputs[0], args.top)
        print_label(top_indices, args.labels, image_path)
    # -run inference


if __name__ == "__main__":

    args = get_args()

    main(args)
