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

import argparse
import os
import pathlib
import sys


def parse_args():
    """
    Parse the common command-line arguments

    Returns:
        argparse.Namespace: args
    """
    parser = argparse.ArgumentParser(description="resnet50 example")

    parser.add_argument(
        "--model",
        default="",
        help="Path to the resnet50 model on the server",
    )

    parser.add_argument(
        "--image",
        default="",
        help="Path to an image or a directory of images on the client",
    )

    parser.add_argument(
        "--labels",
        default="",
        help="Path to the text file containing the labels on the client",
    )

    parser.add_argument(
        "--batch-size",
        default=1,
        type=int,
        help="Batch size to use for the worker on the server",
    )

    parser.add_argument(
        "--ip",
        default="127.0.0.1",
        help="IP to use for server",
    )

    parser.add_argument(
        "--http-port",
        default=8998,
        type=int,
        help="Port to use for HTTP server",
    )

    parser.add_argument(
        "--grpc-port",
        default=50051,
        type=int,
        help="Port to use for gRPC server",
    )

    parser.add_argument(
        "--endpoint",
        default="",
        help="Endpoint to use for inference. If empty, load a worker first",
    )

    parser.add_argument(
        "--top",
        default=5,
        type=int,
        help="Number of top categories to print",
    )

    parser.add_argument(
        "--input-size",
        default=224,
        type=int,
        help="Size of the square image in pixels",
    )

    parser.add_argument(
        "--output-classes",
        default=1000,
        type=int,
        help="Number of output classes for this model",
    )

    parser.add_argument(
        "--input-node",
        default="",
        help="Name of the input node",
    )

    parser.add_argument(
        "--output-node",
        default="",
        help="Name of the output node",
    )

    args = parser.parse_args()

    if (not args.image) or (not args.model) or (not args.labels):
        root = os.getenv("AMDINFER_ROOT")
        if root is None:
            print("AMDINFER_ROOT is not defined in the environment")
            print("-> Needed to infer default values for arguments")
            print("Either:\n - define AMDINFER_ROOT in the environment")
            print(" - pass all the following flags:")
            print("     --image")
            print("     --model")
            print("     --labels")
            sys.exit(1)

        if not args.image:
            args.image = root + "/tests/assets/dog-3619020_640.jpg"

        # args.model is unset and set by each example

        if not args.labels:
            args.labels = root + "/examples/resnet50/imagenet_classes.txt"

    return args


def resolve_image_paths(path: pathlib.Path):
    """
    Convert the input path to a list of paths. If the input path is a directory,
    the returned list contains all the files at that path.

    Args:
        path (pathlib.Path): Input path

    Returns:
        list[str]: list of paths
    """
    images = []
    if path.is_dir():
        for file in path.glob("*"):
            images.append(str(file))
    else:
        images.append(str(path))
    return images


def print_label(indices, path_to_labels, name):
    """
    Print the labels associated with the given indices

    Args:
        indices (list[int]): Indices to print
        path_to_labels (str | pathlib.Path): path to the labels text file
        name (str): string identifier to print with the labels
    """
    with open(path_to_labels, "r") as f:
        lines = f.readlines()

    print(f"Top {len(indices)} classes for {name}:")
    for index in indices:
        print(f"  {lines[index].strip()}")
