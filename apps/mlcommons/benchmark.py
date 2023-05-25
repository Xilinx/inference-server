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
import json
import shlex
import subprocess
import textwrap


class MlcommonsLog:
    def __init__(self, path, model):
        self.valid = False
        self.model = model
        self.scenario = ""
        self.test_mode = ""

        self.qps = 0
        self.min_latency = 0
        self.max_latency = 0
        self.mean_latency = 0
        self.latency_50_0 = 0
        self.latency_90_0 = 0
        self.latency_95_0 = 0
        self.latency_97_0 = 0
        self.latency_99_0 = 0
        self.latency_99_9 = 0

        self._parse_log(path)

    def _set_attribute(self, content, key, attribute):
        if content["key"] == key:
            setattr(self, attribute, content["value"])

    def _parse_log(self, path):
        with open(path, "r") as f:
            for line in f:
                content = json.loads(line.split(":::MLLOG")[1])
                if content["key"] == "result_validity":
                    self.valid = content["value"] == "VALID"
                set_attribute = lambda key, attribute: self._set_attribute(
                    content, key, attribute
                )
                set_attribute("result_qps_without_loadgen_overhead", "qps")
                set_attribute("result_min_latency_ns", "min_latency")
                set_attribute("result_max_latency_ns", "max_latency")
                set_attribute("result_mean_latency_ns", "mean_latency")
                set_attribute("result_50.00_percentile_latency_ns", "latency_50_0")
                set_attribute("result_90.00_percentile_latency_ns", "latency_90_0")
                set_attribute("result_95.00_percentile_latency_ns", "latency_95_0")
                set_attribute("result_97.00_percentile_latency_ns", "latency_97_0")
                set_attribute("result_99.00_percentile_latency_ns", "latency_99_0")
                set_attribute("result_99.90_percentile_latency_ns", "latency_99_9")
                set_attribute("requested_scenario", "scenario")
                set_attribute("requested_test_mode", "test_mode")

    def __str__(self):
        return textwrap.dedent(
            f"""
            MLCommons log({self.model}, {self.scenario}, {self.test_mode}):
                Valid: {str(self.valid)}
                QPS w/o loadgen overhead: {str(self.qps)}
                Min latency (ns): {self.min_latency}
                Max latency (ns): {self.max_latency}
                Mean latency (ns): {self.mean_latency}
                50.0% latency (ns): {self.latency_50_0}
                90.0% latency (ns): {self.latency_90_0}
                95.0% latency (ns): {self.latency_95_0}
                97.0% latency (ns): {self.latency_97_0}
                99.0% latency (ns): {self.latency_99_0}
                99.9% latency (ns): {self.latency_99_9}"""
        )


def run_mlcommons(model: str, scenario: str, executable: str):
    cmd = f"{executable} --scenario {scenario} --model {model}"
    try:
        subprocess.check_call(shlex.split(cmd))
    except subprocess.CalledProcessError as e:
        print(f"Returned {e.returncode} from {cmd}")
        print(f"Output: {e.output}")
        return None

    return MlcommonsLog("mlperf_log_detail.txt", model)


def run(args: argparse.Namespace):
    assert args.models
    assert args.scenarios

    for model in args.models:
        for scenario in args.scenarios:
            log = run_mlcommons(model, scenario, args.executable)
            if log is not None:
                print(log)


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Run mlcommons benchmarks")
    parser.add_argument(
        "--executable",
        default="",
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
    args = parser.parse_args()

    if args.executable:
        run(args)
