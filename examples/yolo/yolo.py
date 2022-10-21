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
    parser = argparse.ArgumentParser(description="yolov4 example")

    parser.add_argument(
        "--model",
        default="",
        help="Path to the yolo model on the server",
    )

    parser.add_argument(
        "--image",
        default="",
        help="Path to an image or a directory of images on the client",
    )

    parser.add_argument(
        "--input-size",
        default=416,
        help="Size of the square image in pixels",
    )

    parser.add_argument(
        "--labels",
        default="",
        help="Path to the text file containing the labels on the client",
    )

    parser.add_argument(
        "--anchors",
        default="",
        help="Path to the text file containing the anchors on the client",
    )

    parser.add_argument(
        "--http-port",
        default=8998,
        help="Port to use for HTTP server",
    )

    parser.add_argument(
        "--grpc-port",
        default=50051,
        help="Port to use for gRPC server",
    )

    args = parser.parse_args()

    if (not args.image) or (not args.model) or (not args.labels) or (not args.anchors):
        root = os.getenv("PROTEUS_ROOT")
        if root is None:
            print("PROTEUS_ROOT is not defined in the environment")
            print("-> Needed to infer default values for arguments")
            print("Either:\n - define PROTEUS_ROOT in the environment")
            print(" - pass all the following flags:")
            print("     --image")
            print("     --model")
            print("     --labels")
            print("     --anchors")
            sys.exit(1)

        if not args.image:
            args.image = root + "/tests/assets/bicycle-384566_640.jpg"

        # args.model is unset and set by each example

        if not args.labels:
            args.labels = root + "/external/artifacts/onnx/yolov4/coco.names"

        if not args.anchors:
            args.anchors = root + "/external/artifacts/onnx/yolov4/yolov4_anchors.txt"

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
