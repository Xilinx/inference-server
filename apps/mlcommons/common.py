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

import json
import textwrap

import pandas as pd


def indent(text: str, indent_length: int, indent_char=" "):
    """
    Indent the text to the specified length

    Args:
        text (str): Text to indent
        indent_length (int): Number of characters to indent
        indent_char (str, optional): Character to use for indent. Defaults to ' '.
    """

    def indented_lines():
        for i, line in enumerate(text.splitlines(True)):
            if i:
                yield indent_char * indent_length + line if line.strip() else line
            else:
                yield line

    return "".join(indented_lines())


class MlcommonsLog:
    def __init__(self, path):
        self._data = {}
        for content, set_attribute in self.parse_log(path):
            if content["key"] == "result_validity":
                self._data["valid"] = content["value"] == "VALID"
            set_attribute("requested_scenario", "scenario")
            set_attribute("requested_test_mode", "test_mode")

            set_attribute("result_min_latency_ns", "min_latency", 1e-9)
            set_attribute("result_max_latency_ns", "max_latency", 1e-9)
            set_attribute("result_mean_latency_ns", "mean_latency", 1e-9)
            set_attribute("result_50.00_percentile_latency_ns", "latency_50_0", 1e-9)
            set_attribute("result_90.00_percentile_latency_ns", "latency_90_0", 1e-9)
            set_attribute("result_95.00_percentile_latency_ns", "latency_95_0", 1e-9)
            set_attribute("result_97.00_percentile_latency_ns", "latency_97_0", 1e-9)
            set_attribute("result_99.00_percentile_latency_ns", "latency_99_0", 1e-9)
            set_attribute("result_99.90_percentile_latency_ns", "latency_99_9", 1e-9)

        self._set_df()

    def _set_df(self):
        self.df = pd.DataFrame(self._data, index=[0])

    def _set_attribute(self, content, key, attribute, scale=1):
        if content["key"] == key:
            if scale == 1:
                self._data[attribute] = content["value"]
            else:
                self._data[attribute] = float(content["value"]) * scale

    def parse_log(self, path):
        with open(path, "r") as f:
            for line in f:
                content = json.loads(line.split(":::MLLOG")[1])
                set_attribute = lambda key, attribute, scale=1: self._set_attribute(
                    content, key, attribute, scale
                )
                yield content, set_attribute

    def __str__(self):
        return str(self.df)


class SingleStream(MlcommonsLog):
    def __init__(self, path):
        super().__init__(path)

        for _, set_attribute in self.parse_log(path):
            set_attribute("result_qps_with_loadgen_overhead", "qps_with_loadgen")
            set_attribute("result_qps_without_loadgen_overhead", "qps")

        self._set_df()


class MultiStream(MlcommonsLog):
    def __init__(self, path):
        super().__init__(path)

        for _, set_attribute in self.parse_log(path):
            set_attribute("result_min_query_latency_ns", "query_min_latency", 1e-9)
            set_attribute("result_max_query_latency_ns", "query_max_latency", 1e-9)
            set_attribute("result_mean_query_latency_ns", "query_mean_latency", 1e-9)
            set_attribute(
                "result_50.00_percentile_per_query_latency_ns",
                "query_latency_50_0",
                1e-9,
            )
            set_attribute(
                "result_90.00_percentile_per_query_latency_ns",
                "query_latency_90_0",
                1e-9,
            )
            set_attribute(
                "result_95.00_percentile_per_query_latency_ns",
                "query_latency_95_0",
                1e-9,
            )
            set_attribute(
                "result_97.00_percentile_per_query_latency_ns",
                "query_latency_97_0",
                1e-9,
            )
            set_attribute(
                "result_99.00_percentile_per_query_latency_ns",
                "query_latency_99_0",
                1e-9,
            )
            set_attribute(
                "result_99.90_percentile_per_query_latency_ns",
                "query_latency_99_9",
                1e-9,
            )

        self._set_df()


class Server(MlcommonsLog):
    def __init__(self, path):
        super().__init__(path)

        for _, set_attribute in self.parse_log(path):
            set_attribute(
                "result_completed_samples_per_sec", "completed_samples_per_sec"
            )
            set_attribute("result_overlatency_query_count", "overlatency_query_count")
        self._set_df()


class Offline(MlcommonsLog):
    def __init__(self, path):
        super().__init__(path)

        for _, set_attribute in self.parse_log(path):
            set_attribute("result_samples_per_second", "samples_per_second")
        self._set_df()


class MlcommonsLogs:
    def __init__(self) -> None:
        self.df = pd.DataFrame()

    def add_log(self, model, protocol, log):
        log.df["model"] = model
        log.df["protocol"] = protocol
        self.df = pd.concat([self.df, log.df], ignore_index=True)

    def __str__(self) -> str:
        return str(self.df)
