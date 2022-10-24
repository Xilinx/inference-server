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
This example demonstrates how you can use the XModel backend to run inference
on an AMD FPGA with a ResNet50 XModel model. Look at the documentation online for
discussion around this example.
"""

import os
import pathlib
import sys
from time import sleep

# +import
import proteus
import proteus.pre_post as pre_post

# -import

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
    options = pre_post.ImagePreprocessOptionsInt8()
    options.order = pre_post.ImageOrder.NHWC
    options.mean = [123, 107, 104]
    options.std = [1, 1, 1]
    options.normalize = True
    return pre_post.imagePreprocessInt8(paths, options)


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
    return pre_post.resnet50PostprocessInt8(output, k)


# +construct request
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


# -construct request


def load(client, args):
    """
    Load a worker to handle an inference request. The load returns the endpoint
    you should use for subsequent requests

    Args:
        client (proteus.Client): the client object
        args (argparse.Namespace): the command line arguments

    Returns:
        str: endpoint
    """

    # Depending on how the server is compiled, it may or may not have support
    # for a particular backend. This guard checks to make sure the server does
    # support the requested backend. If you already know it's supported, you can
    # skip this check.
    metadata = client.serverMetadata()
    if "vitis" not in metadata.extensions:
        print(
            "Vitis AI is not enabled. Please recompile with it enabled to run this example"
        )
        sys.exit(1)

    # Load-time parameters are used to pass one-time information to the batcher
    # and worker as it starts up. Each worker can choose to define its own
    # parameters that it pays attention to. Similarly, the batcher that the worker is
    # using may have its own parameters. Check the documentation to see what may
    # be specified.

    # +load
    parameters = proteus.RequestParameters()
    parameters.put("model", args.model)
    endpoint = client.workerLoad("xmodel", parameters)
    # -load
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
        args.model = (
            root
            + "/external/artifacts/u200_u250/resnet_v1_50_tf/resnet_v1_50_tf.xmodel"
        )

    return args


def main(args):
    print("Running the Vitis example for ResNet50 in Python")

    # +initialize
    server = proteus.Server()
    # -initialize
    print("Waiting until the server is ready...")
    # +start protocol
    server.startHttp(args.http_port)
    # -start protocol

    # + create client
    client = proteus.HttpClient(f"http://127.0.0.1:{args.http_port}")
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
    # in vitis.py
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
