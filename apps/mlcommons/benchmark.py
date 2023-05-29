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

        self.min_latency = 0
        self.max_latency = 0
        self.mean_latency = 0
        self.latency_50_0 = 0
        self.latency_90_0 = 0
        self.latency_95_0 = 0
        self.latency_97_0 = 0
        self.latency_99_0 = 0
        self.latency_99_9 = 0

        for content, set_attribute in self.parse_log(path):
            if content["key"] == "result_validity":
                self.valid = content["value"] == "VALID"
            set_attribute("requested_scenario", "scenario")
            set_attribute("requested_test_mode", "test_mode")

            set_attribute("result_min_latency_ns", "min_latency")
            set_attribute("result_max_latency_ns", "max_latency")
            set_attribute("result_mean_latency_ns", "mean_latency")
            set_attribute("result_50.00_percentile_latency_ns", "latency_50_0")
            set_attribute("result_90.00_percentile_latency_ns", "latency_90_0")
            set_attribute("result_95.00_percentile_latency_ns", "latency_95_0")
            set_attribute("result_97.00_percentile_latency_ns", "latency_97_0")
            set_attribute("result_99.00_percentile_latency_ns", "latency_99_0")
            set_attribute("result_99.90_percentile_latency_ns", "latency_99_9")

    def _set_attribute(self, content, key, attribute):
        if content["key"] == key:
            setattr(self, attribute, content["value"])

    def parse_log(self, path):
        with open(path, "r") as f:
            for line in f:
                content = json.loads(line.split(":::MLLOG")[1])
                set_attribute = lambda key, attribute: self._set_attribute(
                    content, key, attribute
                )
                yield content, set_attribute

    def __str__(self):
        return textwrap.dedent(
            f"""
            MLCommons log({self.model}, {self.scenario}, {self.test_mode}):
                Valid: {str(self.valid)}
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


class SingleStream(MlcommonsLog):
    def __init__(self, path, model):
        super().__init__(path, model)

        self.qps = 0

        for _, set_attribute in self.parse_log(path):
            set_attribute("result_qps_without_loadgen_overhead", "qps")

    def __str__(self):
        return textwrap.dedent(
            f"""
        {super().__str__()}
        QPS w/o loadgen overhead: {str(self.qps)}"""
        )


class MultiStream(MlcommonsLog):
    def __init__(self, path, model):
        super().__init__(path, model)

        self.query_min_latency = 0
        self.query_max_latency = 0
        self.query_mean_latency = 0
        self.query_latency_50_0 = 0
        self.query_latency_90_0 = 0
        self.query_latency_95_0 = 0
        self.query_latency_97_0 = 0
        self.query_latency_99_0 = 0
        self.query_latency_99_9 = 0

        for _, set_attribute in self.parse_log(path):
            set_attribute("result_min_query_latency_ns", "query_min_latency")
            set_attribute("result_max_query_latency_ns", "query_max_latency")
            set_attribute("result_mean_query_latency_ns", "query_mean_latency")
            set_attribute(
                "result_50.00_percentile_per_query_latency_ns", "query_latency_50_0"
            )
            set_attribute(
                "result_90.00_percentile_per_query_latency_ns", "query_latency_90_0"
            )
            set_attribute(
                "result_95.00_percentile_per_query_latency_ns", "query_latency_95_0"
            )
            set_attribute(
                "result_97.00_percentile_per_query_latency_ns", "query_latency_97_0"
            )
            set_attribute(
                "result_99.00_percentile_per_query_latency_ns", "query_latency_99_0"
            )
            set_attribute(
                "result_99.90_percentile_per_query_latency_ns", "query_latency_99_9"
            )

    def __str__(self):
        return textwrap.dedent(
            f"""
        {super().__str__()}
        Min latency per query (ns): {self.query_min_latency}
        Max latency per query (ns): {self.query_max_latency}
        Mean latency per query (ns): {self.query_mean_latency}
        50.0% latency per query (ns): {self.query_latency_50_0}
        90.0% latency per query (ns): {self.query_latency_90_0}
        95.0% latency per query (ns): {self.query_latency_95_0}
        97.0% latency per query (ns): {self.query_latency_97_0}
        99.0% latency per query (ns): {self.query_latency_99_0}
        99.9% latency per query (ns): {self.query_latency_99_9}"""
        )


class Server(MlcommonsLog):
    def __init__(self, path, model):
        super().__init__(path, model)

        self.completed_samples_per_sec = 0
        self.overlatency_query_count = 0

        for _, set_attribute in self.parse_log(path):
            set_attribute(
                "result_completed_samples_per_sec", "completed_samples_per_sec"
            )
            set_attribute("result_overlatency_query_count", "overlatency_query_count")

    def __str__(self):
        return textwrap.dedent(
            f"""
        {super().__str__()}
        Completed samples per sec: {self.completed_samples_per_sec}
        Samples over target latency: {self.overlatency_query_count}"""
        )


class Offline(MlcommonsLog):
    def __init__(self, path, model):
        super().__init__(path, model)

        self.samples_per_second = 0

        for _, set_attribute in self.parse_log(path):
            set_attribute("result_samples_per_second", "samples_per_second")

    def __str__(self):
        return textwrap.dedent(
            f"""
        {super().__str__()}
        Samples per second: {self.samples_per_second}"""
        )


class MlcommonsLogs:
    def __init__(self) -> None:
        self.logs = {}

    def add_log(self, model, scenario, log):
        if model not in self.logs:
            self.logs[model] = {}
        self.logs[model][scenario] = log

    def __str__(self) -> str:
        logs = ""
        for _, scenarios in self.logs.items():
            for _, log in scenarios.items():
                logs += str(log)
        return logs


def run_mlcommons(model: str, scenario: str, executable: str):
    cmd = f"{executable} --scenario {scenario} --model {model}"
    try:
        subprocess.check_call(shlex.split(cmd))
    except subprocess.CalledProcessError as e:
        print(f"Returned {e.returncode} from {cmd}")
        print(f"Output: {e.output}")
        return None

    log_name = "mlperf_log_detail.txt"
    if scenario == "SingleStream":
        return SingleStream(log_name, model)
    if scenario == "MultiStream":
        return MultiStream(log_name, model)
    if scenario == "Server":
        return Server(log_name, model)
    return Offline(log_name, model)


def run(args: argparse.Namespace):
    assert args.models
    assert args.scenarios

    logs = MlcommonsLogs()
    for model in args.models:
        for scenario in args.scenarios:
            log = run_mlcommons(model, scenario, args.executable)
            if log is not None:
                logs.add_log(model, scenario, log)

    print(logs)


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
