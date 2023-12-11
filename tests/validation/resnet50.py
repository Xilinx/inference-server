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
This example brings up the migraphx worker and runs a Resnet50 classification model
on the official imagenet validation set (50,000 images).

The model file and test data is not in the git repo. and must be fetched with git or
    amdinfer get or wget

Model source:
https://github.com/onnx/models/blob/5faef4c33eba0395177850e1e31c4a6a9e634c82/vision/classification/resnet/model/resnet50-v2-7.onnx
https://github.com/onnx/models/blob/5faef4c33eba0395177850e1e31c4a6a9e634c82/vision/classification/resnet/model/resnet50-v2-7.tar.gz

Source of ground truth labels for validation set:
    ILSVRC2012_validation_ground_truth.txt
    https://github.com/mvermeulen/rocm-migraphx/tree/master/datasets/imagenet/val.txt
"""

import argparse
import os
import pathlib
import sys
import time

import cv2
import numpy as np

import amdinfer
import amdinfer.pre_post as pre_post


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
    return pre_post.imagePreprocessFp32(paths, options)


def main(args):

    batch_size = args.batch_size
    model = args.model
    validation_dir = args.validation_dir
    ground_truth_file = args.ground_truth
    labels_file = args.labels
    worker = args.worker

    client = amdinfer.HttpClient("http://127.0.0.1:8998")
    print("waiting for server...", end="")
    start_server = not client.serverLive()
    if start_server:
        print("No server detected. Starting locally...")
        server = amdinfer.Server()
        server.startHttp(8998)
        while not client.serverLive():
            time.sleep(1)
    print("ok.")

    metadata = client.serverMetadata()
    if worker not in metadata.extensions:
        print(f"{worker} support required but not found.")
        sys.exit(0)

    # load worker.  The only parameter the migraphx worker requires is the model file name.
    # It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
    # it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
    # future use.  It will read
    # the array dimensions and data type from the model.

    parameters = amdinfer.ParameterMap()
    parameters.put("model", str(model))
    parameters.put("batch", batch_size)
    parameters.put("timeout", 100)  # ms
    # this call requests the server to either find a running instance of the named
    # worker type, or else create one and initialize it with the parameters.
    print("loading worker with model file ", model)
    endpoint = client.workerLoad("migraphx", parameters)

    # load the labels
    with open(labels_file, "r") as f:
        labels = f.read().split("\n")

    # wait for the worker to load and compile model
    ready = False
    while not ready:
        try:
            ready = client.modelReady(endpoint)
        except ValueError:
            pass

    # list all the *.jpg images in the validation image directory
    files = os.listdir(validation_dir)
    files.sort()

    if str(ground_truth_file) in files:
        files.remove(str(ground_truth_file))

    # Read the "answers" file
    ground_truth = [None] * len(files)
    i = 1
    with open(ground_truth_file, "r") as f:
        while i < len(ground_truth):
            ground_truth[i] = f.readline().split()[1]
            i = i + 1

    #
    #   Loop thru the image set, reading images and putting together inference requests in batches.
    #   Then compare the results with ground truth
    #
    correct_answers = 0
    wrong_answers = 0

    print(f"Starting requests for {len(files)} images")
    request_size = 64
    filenames = []
    index = 0
    for file in files:
        filename = os.path.join(validation_dir, file)
        if os.path.isfile(filename) and filename.find(".JPEG") != -1:
            filenames.append(filename)
            index += 1

        if len(filenames) == request_size:
            images = preprocess(filenames)

            imageReqs = [amdinfer.ImageInferenceRequest(image) for image in images]
            print("Request is ready.  Sending ", len(images), " requests")
            j = 0
            responses = amdinfer.inferAsyncOrdered(client, endpoint, imageReqs)
            for response in responses:
                assert not response.isError(), response.getError()

                # print("Client received inference reply.")
                for output in response.getOutputs():
                    assert output.datatype == amdinfer.DataType.FP32
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
            filenames = []

    print("Done")

    # If this line is commented out, worker persists with doRun thread active, and the entire script
    # can be run again without any reloading taking place
    # client.unload('Migraphx')


def validate_args(args):
    # TODO(varunsh): generalize this script
    # supported_workers = ["vitis", "ptzendnn", "tfzendnn", "migraphx"]
    supported_workers = ["migraphx"]
    if args.worker not in supported_workers:
        raise ValueError(f"Worker must be one of: {' '.join(supported_workers)}")
    if not args.model:
        if args.worker == "vitis":
            args.model = amdinfer.testing.getPathToAsset("u250_resnet50")
        elif args.worker == "ptzendnn":
            args.model = amdinfer.testing.getPathToAsset("pt_resnet50")
        elif args.worker == "tfzendnn":
            args.model = amdinfer.testing.getPathToAsset("tf_resnet50")
        else:
            args.model = amdinfer.testing.getPathToAsset("onnx_resnet50")

    return args


def parse_args():
    def _check_path(value):
        if not value:
            raise argparse.ArgumentError(f"{value} is not a valid path")
        value = pathlib.Path(value)
        if not value.exists():
            raise argparse.ArgumentError(f"Path {str(value)} does not exist")
        return value

    parser = argparse.ArgumentParser(description="Validation for the ResNet50 model")

    parser.add_argument(
        "--batch-size",
        "-b",
        default=64,
        help="Batch size for the worker. Defaults to 64.",
    )

    parser.add_argument(
        "--model",
        "-m",
        default="",
        help="Path to the model on the server",
    )

    parser.add_argument(
        "--validation-dir",
        "-v",
        required=True,
        type=_check_path,
        help="Path to the directory containing validation images",
    )
    parser.add_argument(
        "--ground-truth",
        "-g",
        required=True,
        type=_check_path,
        help="Path to the file containing the correct category results for the validation set",
    )

    parser.add_argument(
        "--labels",
        "-l",
        required=True,
        type=_check_path,
        help="Path to the file containing label names for the model's categories",
    )

    parser.add_argument(
        "--input-size",
        "-i",
        default=224,
        help="The size of the square image",
    )

    parser.add_argument(
        "--worker",
        "-w",
        required=True,
        type=str,
        help="Name of the worker backend to use for inference",
    )

    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    args = validate_args(args)
    main(args)
