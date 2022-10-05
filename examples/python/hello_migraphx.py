#####################################################################################
# The MIT License (MIT)
#
# Copyright (c) 2015-2022 Advanced Micro Devices, Inc. All rights reserved.
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
#####################################################################################

"""
This example contains Python commands necessary to bring up
the migraphx worker and run a Resnet50 classification model on some images.
Then, it calls the migraphx library directly with the same model, and compares results.
The model file in external/artifacts is not in the git repo. and must be fetched with git or with
    proteus get

https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.onnx
https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.tar.gz
"""


import argparse
import json  # json for reading labels file
import os
import sys
import time

import cv2
import numpy as np

import proteus
import proteus.clients

# The make_nxn and preprocess functions are based on an migraphx example at
# AMDMIGraphx/examples/vision/python_resnet50/resnet50_inference.ipynb
# The mean and standard dev. values used for this normalization are
# requirements of the Resnet50 model.


def make_nxn(image, n):
    """
    Crop an image to square and then resize to desired dimension n x n

    Args:
        image (np.array): image to crop
        n (int): desired image size
    """
    width = image.shape[1]
    height = image.shape[0]
    if height > width:
        dif = height - width
        cr = dif // 2
        square = image[(cr + (dif % 2)) : (height - cr), :]
        return cv2.resize(square, (n, n))
    elif width > height:
        dif = width - height
        cr = dif // 2
        square = image[:, (cr + (dif % 2)) : (width - cr)]
        return cv2.resize(square, (n, n))
    else:
        return cv2.resize(image, (n, n))


def preprocess(img_data):
    """
    Normalize array values to the data type, mean and std. dev. required by Resnet50

    Args:
        img_data (np.array): 3 dimensions [channels, rows, cols] with value range 0-255
        the vectors are for RGB images, so images read with OpenCV must have channels
        converted before calling.
    """
    mean_vec = np.array([0.406, 0.456, 0.485])
    stddev_vec = np.array([0.225, 0.224, 0.229])
    norm_img_data = np.zeros(img_data.shape).astype("float32")
    for i in range(img_data.shape[0]):
        norm_img_data[i, :, :] = (img_data[i, :, :] / 255 - mean_vec[i]) / stddev_vec[i]
    return norm_img_data


def run_migraphx(model_name, img, labels):
    """
    Do a migraphx call directly for one image; compare results

    Args:
        model_name (str): path to the model
        img (np.array): image to send
        labels (dict): label strings for resnet
    """
    print("Beginning comparison run-->parsing the model for migraphx...")
    model = migraphx.parse_onnx(model_name)
    model.compile(migraphx.get_target("gpu"))
    print("Ok.")

    # img and img2 are already the correct shape for the Inf Server, but need another dimension for Python API migraphx
    test_img = img
    # add a 4th tensor dimension in first position, expected by migraphx
    test_img = np.expand_dims(test_img, 0)

    # Run the inference
    results = model.run({"data": test_img})
    # Extract the index of the top prediction
    res_npa = np.array(results[0])  # shape of res_npa is (1, 1000)
    max_index = np.argmax(res_npa)
    print(
        "category reported by migraphx is ",
        max_index,
        ".  Match value ",
        res_npa[0][max_index],
        "  This is a picture of a ",
        labels[max_index],
    )


def parse_args():
    """
    Read command line arguments
    """
    root = os.getenv("PROTEUS_ROOT")
    assert root is not None

    # Get the arguments required from the user
    parser = argparse.ArgumentParser(
        description="Example client Proteus Migraphx worker"
    )

    parser.add_argument(
        "--modelfile",
        "-m",
        type=str,
        required=False,
        default=os.path.join(
            root, "external/artifacts/onnx/resnet50v2/resnet50-v2-7.onnx"
        ),
        help="Location of model file on server",
    )

    parser.add_argument(
        "--labels",
        "-l",
        type=str,
        required=False,
        default=os.path.join(root, "tests/assets/imagenet_simple_labels.json"),
        help="The file containing label names for the model's categories",
    )

    parser.add_argument(
        "--image",
        "-i",
        type=str,
        required=False,
        default=os.path.join(root, "tests/assets/dog-3619020_640.jpg"),  # a dog
        help="An image to try inference on.  Use git-lfs to pull image assets",
    )

    parser.add_argument(
        "--image2",
        "-g",
        type=str,
        required=False,
        default=os.path.join(
            root, "tests/assets/bicycle-384566_640.jpg"
        ),  # a bicyclist
        help="A second image to try inference on",
    )

    # Parse arguments
    return parser.parse_args()


def main(args):

    modelname = args.modelfile
    imagename = args.image
    imagename2 = args.image2
    labels_file = args.labels

    client = proteus.clients.HttpClient("http://127.0.0.1:8998")
    print("waiting for server...", end="")
    start_server = not client.serverLive()
    if start_server:
        server = proteus.servers.Server()
        server.startHttp(8998)
        while not client.serverLive():
            time.sleep(1)
    print("ok.")

    metadata = client.serverMetadata()

    if not "migraphx" in metadata.extensions:
        print("MIGraphX support required but not found.")
        sys.exit(0)

    #  load the onnx model to find the input shape, see https://stackoverflow.com/questions/56734576/find-input-shape-from-onnx-file
    #  We assume here that client is on the same file system as the server
    #  This code is applicable to any onnx model, but for resnet50 the required shape could have been hardcoded:   [1, 3, 224, 224]
    shape = []
    import onnx

    model = onnx.load(modelname)
    for input in model.graph.input:
        if input.name == "data":
            print(input.name, end=": ")
            # get type of input tensor
            tensor_type = input.type.tensor_type
            # check if it has a shape:
            if tensor_type.HasField("shape"):
                # iterate through dimensions of the shape:
                for d in tensor_type.shape.dim:
                    # the dimension may have a definite (integer) value or a symbolic identifier or neither:
                    if d.HasField("dim_value"):
                        shape.append(d.dim_value)
            else:
                print("unknown rank", end="")
            print()
            break

    print("This model's shape of input image is ", shape)
    # If only 3 dimensions were found, assume the 0'th dimension was not parsed and insert a 1.
    if len(shape) == 3:
        shape.insert(0, 1)
    if len(shape) != 4:
        print(
            "Unable to read the image dimensions from ",
            modelname,
            ".  Expecting a 4-value shape tensor.",
        )
        exit(-1)

    # Load a picture
    img = cv2.imread(imagename).astype("float32")
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    # Crop to a square, resize
    img = make_nxn(img, shape[2])
    #  Normalize values with values specific to Resnet50
    img = img.transpose(2, 0, 1)
    img = preprocess(img)
    # expected dimensions at this point are (3, 224, 224).  Note that the Resnet model expects
    # the channel dimension (3) to come before the rows and columns, but OpenCV places channels
    # in the last dimension.

    # +load worker.  The only parameter the migraphx worker requires is the model file name.
    # It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
    # it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
    # future use.  It will read
    # the array dimensions and data type from the model.

    parameters = proteus.RequestParameters()
    parameters.put("model", modelname)
    parameters.put("batch", 2)
    parameters.put("timeout", 1000)
    # this call requests the server to either find a running instance of the named
    # worker type, or else create one and initialize it with the parameters.
    worker_name = client.workerLoad("Migraphx", parameters)

    # wait for the worker to load and compile model
    ready = False
    while not ready:
        ready = client.modelReady(worker_name)

    # Load a picture
    img2 = cv2.imread(imagename2).astype("float32")
    img2 = cv2.cvtColor(img2, cv2.COLOR_BGR2RGB)
    img2 = make_nxn(img2, shape[2])
    #  Normalize values with values specific to Resnet50
    img2 = img2.transpose(2, 0, 1)
    img2 = preprocess(img2)

    #
    # create multiple image inference requests and send them all together
    #
    images = [img2, img2]

    print("Creating inference request set...")
    images = [proteus.ImageInferenceRequest(image) for image in images]
    responses = proteus.client_operators.inferAsyncOrdered(client, worker_name, images)
    for response in responses:
        assert not response.isError(), response.getError()

    print("Client received inference reply.")

    # load the labels
    with open(labels_file, "r") as json_data:
        labels = json.load(json_data)

    for response in responses:
        for output in response.getOutputs():
            assert output.datatype == proteus.DataType.FP32
            recv_data = output.getFp32Data()
            # the predicted category is the one with the highest match value
            answer = np.argmax(recv_data)
            print(
                "client's analysis of result: best match category is ",
                answer,
                "  match value is ",
                recv_data[answer],
                ".  This is a picture of a ",
                labels[answer],
            )

    # optional: make the same request to the migraphx API for comparison
    # run_migraphx(modelname, img2, labels)

    print("Done")

    # If this line is commented out, worker persists with doRun thread active, and the entire script
    # can be run again without any reloading taking place
    # client.unload('Migraphx')


if __name__ == "__main__":
    args = parse_args()
    main(args)
