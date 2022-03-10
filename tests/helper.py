# Copyright 2021 Xilinx Inc.
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
import pathlib
import os
import re

root = os.getenv("PROTEUS_ROOT")
if root is None:
    raise RuntimeError("PROTEUS_ROOT not defined in the environment")
root_path = pathlib.Path(root)
try:
    with open(root_path / "build/config.txt", "r") as f:
        build = f.read().strip().split(" ")[0]
except FileNotFoundError:
    print("No config.txt found in build/. Using default value of 'Debug'")
    build = "Debug"
build_path = root_path / f"build/{build}"
run_path = build_path / "src/proteus/proteus-server"


def getConstexprFromHeader(constexpr, file):
    with open(file) as f:
        for line in f:
            if line.startswith(f"constexpr auto {constexpr}"):
                matches = re.match(r".* = (.*);", line)
                if matches:
                    return matches.group(1)


def write_lua(filename: str, data: str):
    lua_file = [
        "done = function(summary, latency, requests)",
        '  io.write("------------------------------\\n")',
        '  io.write(string.format("%u,%u,%f,%f\\n", latency.min, latency.max, latency.mean, latency.stdev))',
        '  io.write(string.format("%u,%u,%f,%f\\n", requests.min, requests.max, requests.mean, requests.stdev))',
        '  io.write(string.format("%u,%u,%u\\n", summary.duration, summary.requests, summary.bytes))',
        "end",
        'wrk.method = "POST"',
        'wrk.headers["Content-Type"] = "application/json"',
        f"wrk.body = '{data}'",
    ]

    with open(root_path / "tests/python" / (filename + ".lua"), "w") as f:
        for line in lua_file:
            f.write(line + "\n")


def run_benchmark_func(benchmark, func, **kwargs):
    benchmark.extra_info["model"] = kwargs["model"]
    for key, value in kwargs.items():
        benchmark.extra_info[key] = value
    benchmark(func)


def run_benchmark(benchmark, benchmark_name, func, request, **kwargs):
    write_lua(benchmark_name, json.dumps(request.asdict()))
    benchmark.extra_info["lua"] = benchmark_name
    benchmark.extra_info["model"] = kwargs["model"]
    for key, value in kwargs.items():
        benchmark.extra_info[key] = value
    benchmark(func, kwargs["model"], request)


kDefaultHttpPort = int(
    getConstexprFromHeader(
        "kDefaultHttpPort", root_path / "include/proteus/build_options.hpp"
    )
)
