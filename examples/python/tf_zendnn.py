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
import functools
import multiprocessing as mp
import os
import sys
import time

import numpy as np
from utils.utils import postprocess, preprocess

import proteus
import proteus.clients


def main(args):
    """
    This is the main method which
    1. Spin up the server and worker if not already up
    2. Preprocess if prediction on real image, otherwise create dummy images
    3. Do the prediction on the data
    4. Post process and prints output if real image (correctness)
    5. Or share the time taken for prediction (functionality)

    Args:
        args (argparse): Parsed argument dictionary

    Raises:
        FileNotFoundError: If graph given in argument is not found
        FileNotFoundError: If image_location is given and the file is not found
    """

    client, server = proteus.start_http_client_server(
        "http://127.0.0.1:8998", "tfzendnn"
    )

    # Argument parsing
    real_data = True if args.image_location else False
    batch_size = args.batch_size
    input_size = args.input_size
    output_classes = args.output_classes

    # Validating if Image and Model are available
    if not os.path.exists(args.graph):
        raise FileNotFoundError(
            f"Model not found in the location: {args.graph}. Please check the parameter"
        )

    if real_data and not os.path.exists(args.image_location):
        raise FileNotFoundError(
            f"Image not found in the location: {args.image_location}. Please check the parameter"
        )

    # If predicting on real image, load class list from file.
    if real_data:
        try:
            with open(args.output_class_file) as f:
                classes = [line.strip() for line in f.readlines()]
        except FileNotFoundError:
            classes = list(range(1000))
        classes = np.asarray(classes)

    # Load the worker with appropriate parameters
    parameters = proteus.RequestParameters()
    parameters.put("model", args.graph)
    parameters.put("input_node", args.input_node)
    parameters.put("output_node", args.output_node)
    parameters.put("input_size", input_size)
    parameters.put("output_classes", output_classes)
    parameters.put("inter_op", args.inter_op)
    parameters.put("intra_op", args.intra_op)
    parameters.put("batch_size", batch_size)
    worker_name = client.workerLoad("TfZendnn", parameters)

    ready = False
    while not ready:
        ready = client.modelReady(worker_name)

    # Inference with images
    # If with real data, do preprocessing, otherwise create dummy data
    if real_data:
        # For tests with single images
        images = [preprocess(args.image_location, input_size, args.resize_method)]
        request = proteus.ImageInferenceRequest(images)
        response = client.modelInfer(worker_name, request)
        assert not response.isError(), response.getError()

        # Post process to get top1 and top5 classes
        idx_1 = postprocess(response, 1)
        idx_5 = postprocess(response, 5)

        # Print top1 and top5 classess
        print("\nImage: {}".format(args.image_location))
        print("Top 1 Class: {}".format(classes[idx_1[0]]))
        print("Top 5 classes: {}".format(", ".join(classes[idx_5])))

    else:
        # For functionality check with multiple images
        num_processed_images = total_time = 0
        num_remaining_images = batch_size * args.steps

        print(
            "Running Inference for {} images ({} images for {} steps)\n".format(
                num_remaining_images, batch_size, args.steps
            )
        )

        # Run the inference for the images
        processes = int(mp.cpu_count() / 2)
        while num_remaining_images >= batch_size:

            # create some random data
            images = np.random.uniform(
                0.0, 255.0, (batch_size, input_size, input_size, 3)
            ).astype(np.float32)
            images = [proteus.ImageInferenceRequest(image) for image in images]

            # Send request to the server
            start = time.time()
            proteus.client_operators.inferAsyncOrdered(client, worker_name, images)
            end = time.time()
            total_time += end - start

            num_processed_images += batch_size
            num_remaining_images -= batch_size

            print(
                "Images Processed: {:6}, Images Left: {:6}".format(
                    num_processed_images, num_remaining_images
                ),
                end="\r",
            )

        print("\n\nTotal Images processed: {}".format(num_processed_images))
        print(
            "Total time taken for prediction (including http delay): {:.2f}s".format(
                total_time
            )
        )

    print("Inference Completed")


if __name__ == "__main__":

    root = os.getenv("PROTEUS_ROOT")
    assert root is not None

    # Get the arguments required from the user
    parser = argparse.ArgumentParser(
        description="Validation (working) for Proteus TF+ZenDNN worker"
    )
    parser.add_argument(
        "--graph",
        "-g",
        type=str,
        required=False,
        help="Full path to the input graph",
        default=os.path.join(
            root, "external/tensorflow_models/resnet_v1_50_baseline_6.96B_922.pb"
        ),
    )
    parser.add_argument(
        "--image_location",
        "-d",
        type=str,
        required=False,
        help="Specify the location of the image.",
    )
    parser.add_argument(
        "--batch_size",
        "-b",
        type=int,
        required=False,
        default=16,
        help="Specify the batch size. Default is 64. "
        "If image location is given, this will be 1",
    )
    parser.add_argument(
        "--input_node",
        "-i",
        type=str,
        required=False,
        default="input",
        help="Specify the name of input node. Default is input",
    )
    parser.add_argument(
        "--output_node",
        "-o",
        type=str,
        required=False,
        default="resnet_v1_50/predictions/Reshape_1",
        help="Specify the name of output node. Default is predict",
    )
    parser.add_argument(
        "--input_size",
        "-s",
        type=int,
        required=False,
        default=224,
        help="Specify the Input Size. Default is 224",
    )
    parser.add_argument(
        "--output_classes",
        "-n",
        type=int,
        required=False,
        default=1000,
        help="Specify the Number of Output Classes. Default is 1000",
    )
    parser.add_argument(
        "--output_class_file",
        "-f",
        type=str,
        required=False,
        default=os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "utils/imagenet_classes.txt"
        ),
        help="If real data is given, then this will help output class names",
    )
    parser.add_argument(
        "--resize_method",
        "-r",
        type=str,
        required=False,
        default="xilinx",
        help="Specify the resize method for images: crop (ResNet50)"
        ", bilinear (MobileNetv1, InceptionV3), xilinx (xilinx ResNet50)",
    )
    parser.add_argument(
        "--inter_op",
        type=int,
        required=False,
        default=1,
        help="Specify the Input Size. Default is 64",
    )
    parser.add_argument(
        "--intra_op",
        type=int,
        required=False,
        default=64,
        help="Specify the Input Size. Default is 1",
    )
    parser.add_argument(
        "--steps",
        type=int,
        required=False,
        default=5,
        help="Maximum number of images (steps * batch size) used for "
        "benchmarking (only used if image_location not provided)",
    )

    # Parse arguments and call main
    args = parser.parse_args()
    main(args)
