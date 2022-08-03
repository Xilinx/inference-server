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
This example brings up the migraphx worker and runs a Resnet50 classification model on the official imagenet validation set (50,000 images).

The model file and test data is not in the git repo. and must be fetched with git or
    proteus get or wget

Model source:
https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.onnx
https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.tar.gz

Source of ground truth labels for validation set:
    ILSVRC2012_validation_ground_truth.txt
    https://github.com/mvermeulen/rocm-migraphx/tree/master/datasets/imagenet/val.txt
"""


import os

import argparse
import json  # json for reading labels file
import sys
import time

import cv2
import numpy as np

import proteus
import proteus.clients


# The make_nxn and preprocess functions are based on an migraphx example at
# AMDMIGraphx/examples/vision/python_resnet50/resnet50_inference.ipynb
# The mean and standard dev. values used for this normalization are requirements of the Resnet50 model.


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
        bar = dif // 2
        square = image[(bar + (dif % 2)) : (height - bar), :]
        return cv2.resize(square, (n, n))
    elif width > height:
        dif = width - height
        bar = dif // 2
        square = image[:, (bar + (dif % 2)) : (width - bar)]
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


def parse_args():
    """
    Read command line arguments
    """

    root = os.getenv("PROTEUS_ROOT")
    assert root is not None

    # Get the arguments required from the user
    parser = argparse.ArgumentParser(
        description="Validation (working) for Proteus Migraphx worker"
    )
    parser.add_argument(
        "--request_size",
        "-r",
        type=int,
        required=False,
        help="Number of images per REST request",
        default=4,
    )

    parser.add_argument(
        "--batch_size",
        "-b",
        type=int,
        required=False,
        default=64,
        help="Batch size for migraphx evaluation. Default is 64. ",
    )
    parser.add_argument(
        "--modelfile",
        "-m",
        type=str,
        required=False,
        default=os.path.join(
            root, "external/artifacts/migraphx/resnet50v2/resnet50-v2-7.onnx"
        ),
        help="Location of model file on server",
    )
    parser.add_argument(
        "--validation_dir",
        "-v",
        type=str,
        required=False,
        default=os.path.join(root, "external/artifacts/migraphx/ILSVRC2012_img_val"),
        help="The directory containing validation images, such as the Imagenet validation set if available",
    )
    parser.add_argument(
        "--groundtruth",
        "-g",
        type=str,
        required=False,
        default="val.txt",
        help="The list of correct category results (ground truth) for the validation set; file must be in the validation directory",
    )

    parser.add_argument(
        "--labels",
        "-l",
        type=str,
        required=False,
        default=os.path.join(root, "tests/assets/imagenet_simple_labels.json"),
        help="The file containing label names for the model's categories",
    )

    return parser.parse_args()


def main(args):

    batch_size = args.batch_size
    request_size = args.request_size
    modelname = args.modelfile
    validation_dir = args.validation_dir
    validation_answers_file = os.path.join(validation_dir, args.groundtruth)
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

    if not os.path.exists(validation_dir):
        print(f"{validation_dir} does not exist, skipping migraphx validation test")
        # exiting with zero to prevent failing automated tests
        sys.exit(0)

    #  load the onnx model to find the input shape, see https://stackoverflow.com/questions/56734576/find-input-shape-from-onnx-file
    #  We assume here that client is on the same file system as the server
    #  This code is applicable to any onnx model, but for resnet50 the required shape could have been hardcoded:   [1, 3, 224, 224]
    shape = []
    import onnx

    print('Loading model to verify data shape...')
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
        sys.exit(-1)

    # load worker.  The only parameter the migraphx worker requires is the model file name.
    # It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
    # it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
    # future use.  It will read
    # the array dimensions and data type from the model.

    parameters = proteus.RequestParameters()
    parameters.put("model", modelname)
    parameters.put("batch", batch_size)
    parameters.put("timeout", 100)  # ms
    # this call requests the server to either find a running instance of the named
    # worker type, or else create one and initialize it with the parameters.
    print("loading worker Migraphx with model file ", modelname)
    worker_name = client.workerLoad("Migraphx", parameters)

    # load the labels
    with open(labels_file, "r") as json_data:
        labels = json.load(json_data)

    # wait for the worker to load and compile model
    ready = False
    while not ready:
        try:
            ready = client.modelReady(worker_name)
        except ValueError:
            pass

    # list all the *.jpg images in the validation image directory
    files = os.listdir(validation_dir)
    files.sort()
    files.remove("val.txt")

    # Read the "answers" file
    ground_truth = [None] * len(files)
    i = 1
    with open(validation_answers_file, "r") as labelfile:
        while i < len(ground_truth):
            ground_truth[i] = labelfile.readline().split()[1]
            i = i + 1

    #
    #   Loop thru the image set, reading images and putting together inference requests in batches.
    #   Then save the results and compare with ground truth
    #
    results = [None] * len(files)
    correct_answers = 0
    wrong_answers = 0

    images = [None] * request_size
    index = 0
    for file in files:
        filename = os.path.join(validation_dir, file)
        if os.path.isfile(filename) and filename.find(".JPEG") != -1:
            print("file ", file)
            # Load a picture
            imgV = cv2.imread(filename).astype("float32")
            imgV = cv2.cvtColor(imgV, cv2.COLOR_BGR2RGB)
            imgV = make_nxn(imgV, shape[2])
            #  Normalize values with values specific to Resnet50
            imgV = imgV.transpose(2, 0, 1)
            imgV = preprocess(imgV)
            images[index % request_size] = imgV
            index = index + 1
            if index % request_size == 0:
                print("processed so far: ", index, " of ", len(files))
                print("Creating inference request with ", len(images), " items")
                request = proteus.ImageInferenceRequest(images, False)
                print("request is ready.  Sending...")
                response = client.modelInfer(worker_name, request)
                assert not response.isError(), response.getError()
                print("Client received inference reply.")
                j = 0
                for output in response.getOutputs():
                    assert output.datatype == proteus.DataType.FP32
                    recv_data = output.getFp32Data()
                    # the predicted category is the one with the highest match value
                    answer = np.argmax(recv_data)
                    step_index = index - request_size + j + 1
                    print(
                        "Reading result: best match category is ",
                        answer,
                        "  value is ",
                        recv_data[answer],
                        ".  This is a picture of a ",
                        labels[answer],
                        "    ground truth: ",
                        ground_truth[step_index],
                    )
                    if int(answer) == int(ground_truth[step_index]):
                        correct_answers = correct_answers + 1.0
                    else:
                        wrong_answers = wrong_answers + 1.0
                    j = j + 1
                print(
                    "Correct: ",
                    correct_answers,
                    "  Wrong: ",
                    wrong_answers,
                    "    Accuracy: ",
                    correct_answers / (correct_answers + wrong_answers),
                )
                print("     ----------------------------------------")

    print("Done")

    # If this line is commented out, worker persists with doRun thread active, and the entire script
    # can be run again without any reloading taking place
    # client.unload('Migraphx')


if __name__ == "__main__":
    args = parse_args()
    main(args)
