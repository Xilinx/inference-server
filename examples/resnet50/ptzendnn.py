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
This example demonstrates how you can use the PT+ZenDNN backend to run inference
on an AMD CPU with a ResNet50 PyTorch model. Look at the documentation online for
discussion around this example.
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
    """
    Given a list of paths to images, preprocess the images and return them

    Args:
        paths (list[str]): Paths to images

    Returns:
        list[numpy.ndarray]: List of images
    """
    options = pre_post.ImagePreprocessOptionsFloat()
    options.order = pre_post.ImageOrder.NCHW
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
    """
    Postprocess the output data. For ResNet50, this includes performing a softmax
    to determine the most probable classifications

    Args:
        output (proteus.InferenceResponseOutput): the output from the inference server
        k (int): number of top categories to return

    Returns:
        list[int]: indices for the top k categories
    """
    return pre_post.resnet50PostprocessFloat(output, k)


def construct_requests(images):
    """
    Construct requests for the inference server from the input images. For ResNet50,
    a valid request includes a single input tensor containing a square image.

    Args:
        images (list[numpy.ndarray]): the input images

    Returns:
        list[proteus.InferenceRequest]: the requests
    """
    requests = []
    for image in images:
        requests.append(proteus.ImageInferenceRequest(image))
    return requests


def load(client, args):
    """
    Load a worker to handle an inference request. The load returns the endpoint
    you should use for subsequent requests

    Args:
        client (proteus.client.Client): the client object
        args (argparse.Namespace): the command line arguments

    Returns:
        str: endpoint
    """
    # Depending on how the server is compiled, it may or may not have support
    # for a particular backend. This guard checks to make sure the server does
    # support the requested backend. If you already know it's supported, you can
    # skip this check.

    metadata = client.serverMetadata()
    if "ptzendnn" not in metadata.extensions:
        print(
            "PT+ZenDNN is not enabled. Please recompile with it enabled to run this example"
        )
        sys.exit(1)

    # Load-time parameters are used to pass one-time information to the batcher
    # and worker as it starts up. Each worker can choose to define its own
    # parameters that it pays attention to. Similarly, the batcher the worker is
    # using may have its own parameters. Check the documentation to see what may
    # be specified.

    parameters = proteus.RequestParameters()
    parameters.put("model", args.model)
    parameters.put("input_size", args.input_size)
    parameters.put("output_classes", args.output_classes)
    endpoint = client.workerLoad("ptzendnn", parameters)
    return endpoint


def get_args():
    """
    The command-line arguments are parsed in two phases. There's the common
    arguments that are initialized by parseArgs that are shared by all the
    Python examples in this directory and then example-specific settings are
    initialized here

    Returns:
        argparse.Namespace: the args
    """
    args = parse_args()

    if not args.model:
        root = os.getenv("PROTEUS_ROOT")
        args.model = root + "/external/artifacts/pytorch/resnet50_pretrained.pt"

    return args


def main(args):
    print("Running the PT+ZenDNN example for ResNet50 in Python")

    server = proteus.servers.Server()
    print("Waiting until the server is ready...")
    server.startHttp(args.http_port)

    client = proteus.clients.HttpClient(f"http://127.0.0.1:{args.http_port}")
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
    responses = proteus.client_operators.inferAsyncOrdered(client, endpoint, requests)
    print("Making inferences...")
    for image_path, response in zip(paths, responses):
        assert not response.isError()

        outputs = response.getOutputs()
        assert len(outputs) == 1
        top_indices = postprocess(outputs[0], args.top)
        print_label(top_indices, args.labels, image_path)


if __name__ == "__main__":

    args = get_args()

    main(args)
