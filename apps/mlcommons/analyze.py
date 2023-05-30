# Copyright 2023 Advanced Micro Devices, Inc.
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
import pickle

import common
import plotly.express as px


def graph_protocols(logs: common.MlcommonsLogs):
    for key in logs:
        model, scenario, protocol, log = key


def main(args: argparse.Namespace):
    with open(args.data, "rb") as f:
        logs = pickle.load(f)

    if args.print:
        print(logs)

    graph_protocols(logs)


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Run mlcommons benchmarks")
    parser.add_argument(
        "--data",
        default="mlcommons.bin",
        help="Path to file to analyze. Defaults to mlcommons.bin",
    )
    parser.add_argument(
        "--print",
        action="store_true",
        help="Print the raw data",
    )
    args = parser.parse_args()

    main(args)
