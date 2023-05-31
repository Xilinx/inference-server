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
import itertools
import pickle
import shlex
import subprocess

import common


def run_mlcommons(model: str, scenario: str, protocol: str, executable: str):
    cmd = f"{executable} --scenario {scenario} --model {model} --protocol {protocol}"
    print(f"Running {cmd}")
    try:
        subprocess.check_call(shlex.split(cmd))
    except subprocess.CalledProcessError as e:
        print(f"Failed to run: returned {e.returncode} from {cmd}")
        print(f"    Output: {e.output}")
        return None

    log_name = "mlperf_log_detail.txt"
    if scenario == "SingleStream":
        return common.SingleStream(log_name)
    if scenario == "MultiStream":
        return common.MultiStream(log_name)
    if scenario == "Server":
        return common.Server(log_name)
    return common.Offline(log_name)


def run(args: argparse.Namespace):
    assert args.models
    assert args.scenarios

    logs = common.MlcommonsLogs()
    for model, scenario, protocol in itertools.product(
        args.models, args.scenarios, args.protocols
    ):
        log = run_mlcommons(model, scenario, protocol, args.executable)
        if log is not None:
            logs.add_log(model, scenario, protocol, log)
            with open(args.data, "wb") as f:
                pickle.dump(logs, f)


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Run mlcommons benchmarks")
    parser.add_argument(
        "executable",
        help="Path to the mlcommons app",
    )
    parser.add_argument(
        "--scenarios",
        nargs="+",
        help="Scenarios to run",
        choices={"SingleStream", "MultiStream", "Server", "Offline"},
        required=True,
    )
    parser.add_argument(
        "--models",
        nargs="+",
        help="Models to run",
        choices={"resnet50", "retinanet", "3d-unet", "rnnt", "bert", "dlrm", "fake"},
        required=True,
    )
    parser.add_argument(
        "--protocols",
        nargs="+",
        help="Protocols to run",
        choices={"http", "grpc", "native"},
        required=True,
    )
    parser.add_argument(
        "--data",
        default="mlcommons.bin",
        help="Path to file to save data to while running. Defaults to mlcommons.bin",
    )
    args = parser.parse_args()

    if args.executable:
        run(args)
