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
import sys


def parse_args():
    """
    Parse the common command-line arguments

    Returns:
        argparse.Namespace: args
    """
    parser = argparse.ArgumentParser(description="bert example")

    parser.add_argument(
        "--model",
        default="",
        help="Path to the bert model on the server",
    )

    parser.add_argument(
        "--input",
        default="",
        help="Path to a JSON file containing the inputs on the client",
    )

    parser.add_argument(
        "--vocab",
        default="",
        help="Path to the text file containing the vocabulary on the client",
    )

    parser.add_argument(
        "--batch-size",
        default=10,
        help="Batch size to use for the MIGraphX worker on the server",
    )

    parser.add_argument(
        "--ip",
        default="127.0.0.1",
        help="IP to use for server",
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

    if (not args.input) or (not args.model) or (not args.vocab):
        root = os.getenv("AMDINFER_ROOT")
        if root is None:
            print("AMDINFER_ROOT is not defined in the environment")
            print("-> Needed to infer default values for arguments")
            print("Either:\n - define AMDINFER_ROOT in the environment")
            print(" - pass all the following flags:")
            print("     --input")
            print("     --model")
            print("     --vocab")
            sys.exit(1)

        # args.input is unset and set by each example
        # args.model is unset and set by each example
        # args.vocab is unset and set by each example

    return args
