# MIT License
#
# Copyright (c) 2022 Advanced Micro Devices, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


"""
This example demonstrates how you can use the MIGraphX backend to run inference
on an AMD GPU with a YOLOv4 ONNX model. Look at the documentation online for
discussion around this example.

This example is based on a similar example in MIGraphX:
https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/blob/develop/examples/vision/python_yolov4/yolov4_inference.ipynb
"""

import os
import pathlib
import sys
import time

import cv2
import numpy as np
from PIL import Image

import proteus
import proteus.clients

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
    if "migraphx" not in metadata.extensions:
        print(
            "MIGraphX is not enabled. Please recompile with it enabled to run this example"
        )
        sys.exit(1)

    # Load-time parameters are used to pass one-time information to the batcher
    # and worker as it starts up. Each worker can choose to define its own
    # parameters that it pays attention to. Similarly, the batcher the worker is
    # using may have its own parameters. Check the documentation to see what may
    # be specified.

    # +load worker.  The only parameter the migraphx worker requires is the model file name.
    # batch and timeout are optional.
    # It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
    # it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
    # future use.  It will read the array dimensions and data type from the model.
    parameters = proteus.RequestParameters()
    parameters.put("model", args.model)

    # I found that allocation could fail with a large batch value of 64 and large
    # (13) default buffer count in the migraphx worker
    # Beyond batch size 56, the worker seems to lock up while compiling the model
    parameters.put("batch", 2)

    # this call requests the server to either find a running instance of the named
    # worker type, or else create one and initialize it with the parameters.
    endpoint = client.workerLoad("migraphx", parameters)

    # wait for the worker to load and compile model
    ready = False
    while not ready:
        ready = client.modelReady(endpoint)

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
        root = os.getenv("PROTEUS_ROOT")
        args.model = root + "/external/artifacts/onnx/yolov4/yolov4.onnx"

    return args


def main(args):
    print("Running the MIGraphX example for Yolo in Python")

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
        time.sleep(1)

    print("Loading worker...")
    endpoint = load(client, args)

    print("Preprocessing...")
    paths = resolve_image_paths(pathlib.Path(args.image))
    images, original_images = preprocess(paths, args.input_size)

    print("Creating inference requests...")
    requests = [proteus.ImageInferenceRequest(image) for image in images]
    responses = proteus.client_operators.inferAsyncOrdered(client, endpoint, requests)
    print("Client received inference reply")

    assert len(responses) == len(original_images)

    for it, response in enumerate(responses):
        assert not response.isError(), response.getError()

        detections = []
        for out in response.getOutputs():
            assert out.datatype == proteus.DataType.FP32
            this_detect = np.array(out.getFp32Data())
            newshape = out.shape
            # add a 0'th dimension of 1, to make 5 (the migraphx worker stripped
            # off the batch size)
            newshape.insert(0, 1)
            this_detect = this_detect.reshape(newshape)
            detections.append(this_detect)

        # Post-process the model outputs and display image with detection bounding boxes
        STRIDES = [8, 16, 32]
        XYSCALE = [1.2, 1.1, 1.05]

        anchors = ip.get_anchors(args.anchors)
        STRIDES = np.array(STRIDES)

        pred_bbox = ip.postprocess_bbbox(detections, anchors, STRIDES, XYSCALE)

        original_image = original_images[it]
        original_image_size = original_image.shape[:2]
        bounding_boxes = ip.postprocess_boxes(
            pred_bbox, original_image_size, args.input_size, 0.25
        )
        bounding_boxes = ip.nms(bounding_boxes, 0.213, method="nms")
        image = ip.draw_bbox(
            original_image,
            bounding_boxes,
            args.labels,
        )

        image = Image.fromarray(image)
        base_path = pathlib.Path(__file__).parent.resolve()
        output_name = str(base_path / str(it)) + ".jpg"
        image.save(output_name)

        print("Your marked-up image is at " + str(output_name))


if __name__ == "__main__":
    args = get_args()

    main(args)
