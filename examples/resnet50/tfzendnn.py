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
This example demonstrates how you can use the TF+ZenDNN backend to run inference
on an AMD CPU with a ResNet50 TensorFlow model. Look at the documentation online
for discussion around this example.
"""

import os
import pathlib
import sys
from time import sleep

import cv2

import amdinfer
import amdinfer.pre_post as pre_post

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
    options.convert_color = True
    options.color_code = cv2.COLOR_BGR2RGB
    options.assign = True
    return pre_post.imagePreprocessFloat(paths, options)


def postprocess(output, k):
    """
    Postprocess the output data. For ResNet50, this includes performing a softmax
    to determine the most probable classifications

    Args:
        output (amdinfer.InferenceResponseOutput): the output from the inference server
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
        list[amdinfer.InferenceRequest]: the requests
    """
    requests = []
    for image in images:
        requests.append(amdinfer.ImageInferenceRequest(image))
    return requests


def load(client, args):
    """
    Load a worker to handle an inference request. The load returns the endpoint
    you should use for subsequent requests

    Args:
        client (amdinfer.Client): the client object
        args (argparse.Namespace): the command line arguments

    Returns:
        str: endpoint
    """

    # Depending on how the server is compiled, it may or may not have support
    # for a particular backend. This guard checks to make sure the server does
    # support the requested backend. If you already know it's supported, you can
    # skip this check.
    if not amdinfer.serverHasExtension(client, "tfzendnn"):
        print(
            "TF+ZenDNN is not enabled. Please recompile with it enabled to run this example"
        )
        sys.exit(0)

    # Load-time parameters are used to pass one-time information to the batcher
    # and worker as it starts up. Each worker can choose to define its own
    # parameters that it pays attention to. Similarly, the batcher that the worker is
    # using may have its own parameters. Check the documentation to see what may
    # be specified.

    parameters = amdinfer.RequestParameters()
    parameters.put("model", args.model)
    parameters.put("input_size", args.input_size)
    parameters.put("output_classes", args.output_classes)
    parameters.put("input_node", args.input_node)
    parameters.put("output_node", args.output_node)
    parameters.put("batch_size", args.batch_size)
    endpoint = client.workerLoad("tfzendnn", parameters)
    amdinfer.waitUntilModelReady(client, endpoint)
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
        root = os.getenv("AMDINFER_ROOT")
        assert root is not None
        args.model = (
            root + "/external/artifacts/tensorflow/resnet_v1_50_baseline_6.96B_922.pb"
        )

    if not args.input_node:
        args.input_node = "input"

    if not args.output_node:
        args.output_node = "resnet_v1_50/predictions/Reshape_1"

    return args


def main(args):
    print("Running the TF+ZenDNN example for ResNet50 in Python")

    server_addr = f"{args.ip}:{args.grpc_port}"
    client = amdinfer.GrpcClient(server_addr)
    # start it locally if it doesn't already up if the IP address is the localhost
    if args.ip == "127.0.0.1" and not client.serverLive():
        print("No server detected. Starting locally...")
        server = amdinfer.Server()
        server.startGrpc(args.grpc_port)
    elif not client.serverLive():
        raise ConnectionError(f"Could not connect to server at {server_addr}")
    print("Waiting until the server is ready...")
    amdinfer.waitUntilServerReady(client)

    print("Loading worker...")
    endpoint = load(client, args)

    paths = resolve_image_paths(pathlib.Path(args.image))

    images = preprocess(paths)

    requests = construct_requests(images)

    assert len(paths) == len(requests)
    print("Making inferences...")
    for image_path, request in zip(paths, requests):
        response = client.modelInfer(endpoint, request)
        assert not response.isError()

        outputs = response.getOutputs()
        assert len(outputs) == 1
        top_indices = postprocess(outputs[0], args.top)
        print_label(top_indices, args.labels, image_path)


if __name__ == "__main__":

    args = get_args()

    main(args)
