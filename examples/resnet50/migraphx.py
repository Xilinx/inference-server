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
This example demonstrates how you can use the MIGraphX backend to run inference
on an AMD GPU with a ResNet50 ONNX model. Look at the documentation online for
discussion around this example.
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

    # This example uses a custom image preprocessing function implemented in C++
    # You may use any preprocessing logic or skip it entirely if your input data
    # is already preprocessed
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
    if not amdinfer.serverHasExtension(client, "migraphx"):
        print(
            "MIGraphX is not enabled. Please recompile with it enabled to run this example"
        )
        sys.exit(0)

    # Load-time parameters are used to pass one-time information to the batcher
    # and worker as it starts up. Each worker can choose to define its own
    # parameters that it pays attention to. Similarly, the batcher that the worker is
    # using may have its own parameters. Check the documentation to see what may
    # be specified.

    parameters = amdinfer.RequestParameters()
    timeout_ms = 1000  # batcher timeout value in milliseconds

    # Required: specifies path to the model on the server for it to open
    parameters.put("model", args.model)
    # Optional: request a particular batch size to be sent to the backend. The
    # server will attempt to coalesce incoming requests into a single batch of
    # this size and pass it all to the backend.
    parameters.put("batch", args.batch_size)
    # Optional: specifies how long the batcher should wait for more requests before
    # sending the batch on
    parameters.put("timeout", timeout_ms)
    endpoint = client.workerLoad("migraphx", parameters)
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
        args.model = root + "/external/artifacts/onnx/resnet50v2/resnet50-v2-7.onnx"

    return args


def main(args):
    print("Running the MIGraphX example for ResNet50 in Python")

    server_addr = f"http://{args.ip}:{args.http_port}"
    client = amdinfer.HttpClient(server_addr)
    # start it locally if it doesn't already up if the IP address is the localhost
    if args.ip == "127.0.0.1" and not client.serverLive():
        print("No server detected. Starting locally...")
        server = amdinfer.Server()
        server.startHttp(args.http_port)
    elif not client.serverLive():
        raise ConnectionError(f"Could not connect to server at {server_addr}")
    print("Waiting until the server is ready...")
    amdinfer.waitUntilServerReady(client)

    if args.endpoint:
        endpoint = args.endpoint
        if not client.modelReady(endpoint):
            raise ValueError(
                f"Model at {endpoint} does not exist or isn't ready. Verify the endpoint or omit the --endpoint flag to load a new worker"
            )
    else:
        print("Loading worker...")
        endpoint = load(client, args)

    paths = resolve_image_paths(pathlib.Path(args.image))

    images = preprocess(paths)

    requests = construct_requests(images)

    assert len(paths) == len(requests)
    # +run inference
    # in migraphx.py
    responses = amdinfer.inferAsyncOrdered(client, endpoint, requests)
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
