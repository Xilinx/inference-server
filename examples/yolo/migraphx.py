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
on an AMD GPU with a YOLOv4 ONNX model. Look at the documentation online for
discussion around this example.

This example, as compared to the ResNet50 one, is very similar. However, the YOLO
model has multiple outputs unlike ResNet50's single output tensor.

This example is based on a similar example in MIGraphX:
https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/blob/develop/examples/vision/python_yolov4/yolov4_inference.ipynb
"""

import os
import pathlib
import sys
import time

try:
    import cv2
    import numpy as np
    from PIL import Image
except ImportError:
    print(
        "Could not import one or more modules in yolo. Did you run 'pip install -r requirements.txt'?"
    )
    sys.exit(1)


import amdinfer

# isort: split

import yolo_image_processing as ip
from yolo import parse_args, resolve_image_paths


def preprocess(paths, input_size):
    """
    Given a list of paths to images, preprocess the images and return them

    Args:
        paths (list[str]): Paths to images
        input_size (int): Size of the square image in pixels

    Returns:
        list[numpy.ndarray]: List of images
    """

    processed_images = []
    original_images = []
    for path in paths:
        original_image = cv2.imread(path)
        original_image = cv2.cvtColor(original_image, cv2.COLOR_BGR2RGB)
        original_images.append(original_image)

        img = ip.image_preprocess(np.copy(original_image), [input_size, input_size])
        img = img[np.newaxis, ...].astype(np.float32)
        processed_images.append(img)

    return processed_images, original_images


def load(client, args):
    """
    Load a worker to handle an inference request. The load returns the endpoint
    you should use for subsequent requests

    Args:
        client (amdinfer.client.Client): the client object
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

    # The only parameter the migraphx worker requires is the model file name.
    # batch and timeout are optional.
    # It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
    # it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
    # future use.  It will read the array dimensions and data type from the model.
    parameters = amdinfer.ParameterMap()
    parameters.put("model", args.model)

    # bpickrel: I found that allocation could fail with a large batch value of 64
    # and large (13) default buffer count in the migraphx worker
    # Beyond batch size 56, the worker seems to lock up while compiling the model
    parameters.put("batch", args.batch_size)

    # this call requests the server to either find a running instance of the named
    # worker type, or else create one and initialize it with the parameters.
    endpoint = client.workerLoad("migraphx", parameters)

    # wait for the worker to load and compile model
    amdinfer.waitUntilModelReady(client, endpoint)

    return endpoint


def get_args():
    """
    The command-line arguments are parsed in two phases. There's the common
    arguments that are initialized by parse_args that are shared by all the
    Python examples in this directory and then example-specific settings are
    initialized here

    Returns:
        argparse.Namespace: the args
    """
    args = parse_args()

    if not args.model:
        root = os.getenv("AMDINFER_ROOT")
        assert root is not None
        args.model = root + "/external/artifacts/onnx/yolov4/yolov4.onnx"

    return args


def main(args):
    print("Running the MIGraphX example for Yolo in Python")

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

    print("Preprocessing...")
    paths = resolve_image_paths(pathlib.Path(args.image))
    images, original_images = preprocess(paths, args.input_size)

    print("Creating inference requests...")
    requests = [amdinfer.ImageInferenceRequest(image) for image in images]
    responses = amdinfer.inferAsyncOrdered(client, endpoint, requests)
    print("Client received inference reply")

    assert len(responses) == len(original_images)

    for it, response in enumerate(responses):
        assert not response.isError(), response.getError()

        detections = []
        # YOLO produces multiple output tensors and so they all need to be processed
        for out in response.getOutputs():
            assert out.datatype == amdinfer.DataType.FP32
            this_detect = np.array(out.getFp32Data())
            newshape = out.shape
            # add a 0'th dimension of 1, to make 5 (the migraphx worker stripped
            # off the batch size)
            newshape.insert(0, 1)
            this_detect = this_detect.reshape(newshape)
            detections.append(this_detect)

        image = ip.image_postprocess(detections, original_images[it], args)

        image = Image.fromarray(image)
        base_path = pathlib.Path(__file__).parent.resolve()
        output_name = str(base_path / f"yolo_output{str(it)}") + ".jpg"
        image.save(output_name)

        print("Your marked-up image is at " + str(output_name))


if __name__ == "__main__":
    args = get_args()

    main(args)
