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

import argparse
import copy
from enum import Enum, auto
import functools
import json
import itertools
import math
import os
import pathlib
import statistics
import subprocess
import sys
from typing import Optional

from proteus.server import Server
from proteus.rest import Client
import pytest_benchmark.utils
from _pytest.mark import KeywordMatcher
from _pytest.mark.expression import Expression
from rich.console import Console
from rich.table import Table
from rich.progress import Progress
import yaml


class Highlight(Enum):
    smallest = auto()
    largest = auto()
    none = auto()


class Options:
    def __init__(self, options) -> None:
        self.options = options

    def __getattr__(self, name):
        if name in self.options:
            return self.options[name]
        raise AttributeError(f"'Options' object has no attribute '{name}'")


class Config:
    def __init__(self, path) -> None:
        with open(path, "r") as f:
            config = yaml.safe_load(f)
        self.benchmarks = config["benchmarks"]
        self.repeat = config["repeat_count"]
        self.verbosity = config["verbosity"]

        self.pytest = Options(config["pytest"])
        self.wrk = Options(config["wrk"])
        self.cpp = Options(config["cpp"])


class Benchmark:
    """
    This class holds a single group's benchmark results. Each statistic and
    metadata associated with a benchmark is stored in lists internally so it
    can be plotted in order when printing the table to the console.
    """

    def __init__(self, benchmark: dict) -> None:
        """
        Initialize the internal lists using the stats from the initial benchmark
        used to create this object

        Args:
            benchmark (dict): The dict defining the benchmark. This is in the
            same format as output from pytest_benchmark.
        """
        load = benchmark["load"]
        benchmark_type = benchmark["type"]
        benchmark_config = benchmark["config"]
        self.stats = {
            "load": [load],
            "type": [benchmark_type],
            "config": [benchmark_config],
            "name": [benchmark["name"]],
            "min": [benchmark["stats"]["min"]],
            "max": [benchmark["stats"]["max"]],
            "mean": [benchmark["stats"]["mean"]],
            "stddev": [benchmark["stats"]["stddev"]],
            "rounds": [benchmark["stats"]["rounds"]],
            "median": [benchmark["stats"]["median"]],
            "iqr": [benchmark["stats"]["iqr"]],
            "q1": [benchmark["stats"]["q1"]],
            "q3": [benchmark["stats"]["q3"]],
            "iqr_outliers": [benchmark["stats"]["iqr_outliers"]],
            "stddev_outliers": [benchmark["stats"]["stddev_outliers"]],
            "outliers": [benchmark["stats"]["outliers"]],
            "ld15iqr": [benchmark["stats"]["ld15iqr"]],
            "hd15iqr": [benchmark["stats"]["hd15iqr"]],
            "ops": [benchmark["stats"]["ops"]],
            "ops_uncertainty": [benchmark["stats"]["ops_uncertainty"]],
            "total": [benchmark["stats"]["total"]],
            "iterations": [benchmark["stats"]["iterations"]],
        }
        self._final_stats = {}

    def add(self, benchmark: dict):
        """
        Add a benchmark to this group

        Args:
            benchmark (dict): The dict defining the benchmark. This is in the
            same format as output from pytest_benchmark.
        """
        load = benchmark["load"]
        benchmark_type = benchmark["type"]
        benchmark_config = benchmark["config"]
        self.stats["name"].append(benchmark["name"])
        self.stats["load"].append(load)
        self.stats["type"].append(benchmark_type)
        self.stats["config"].append(benchmark_config)
        for key in self.stats.keys():
            try:
                self.stats[key].append(benchmark["stats"][key])
            except KeyError:
                continue

    @staticmethod
    def _data_format(arg):
        """
        Define the highlighting and formatting rules for the different metadata
        in the benchmarks

        Args:
            arg (str): The metadata type to format (e.g. load, min, max etc.)

        Returns:
            partial([data]): A function that accepts [data] of the type of arg and
            returns it as a list of formatted strings for rich to print out
        """
        data_format = {
            "load": {"highlight": Highlight.none, "format": "d"},
            "name": {"highlight": Highlight.none, "format": "s"},
            "type": {"highlight": Highlight.none, "format": "s"},
            "config": {"highlight": Highlight.none, "format": "s"},
            "min": {"highlight": Highlight.smallest, "format": "1.2e"},
            "max": {"highlight": Highlight.smallest, "format": "1.2e"},
            "mean": {"highlight": Highlight.smallest, "format": "1.2e"},
            "stddev": {"highlight": Highlight.smallest, "format": "1.2e"},
            "rounds": {"highlight": Highlight.none, "format": "1.2e"},
            "median": {"highlight": Highlight.smallest, "format": "1.2e"},
            "iqr": {"highlight": Highlight.smallest, "format": "1.2e"},
            "q1": {"highlight": Highlight.smallest, "format": "1.2e"},
            "q3": {"highlight": Highlight.smallest, "format": "1.2e"},
            "iqr_outliers": {"highlight": Highlight.none, "format": "1.2e"},
            "stddev_outliers": {"highlight": Highlight.none, "format": "1.2e"},
            "outliers": {"highlight": Highlight.none, "format": "1.2e"},
            "ld15iqr": {"highlight": Highlight.none, "format": "1.2e"},
            "hd15iqr": {"highlight": Highlight.none, "format": "1.2e"},
            "ops": {"highlight": Highlight.largest, "format": ",.2f"},
            "ops_uncertainty": {"highlight": Highlight.smallest, "format": ",.2f"},
            "total": {"highlight": Highlight.none, "format": "1.2e"},
            "iterations": {"highlight": Highlight.none, "format": "1.2e"},
        }

        def format_func(colors, data):
            """
            Formats the list of data using the given colors to highlight the min
            or max if formatting is enabled for the data type

            Args:
                colors (iterable): Iterable of colors (first is used for min, second for max) or None
                data (iterable): List of data to format

            Returns:
                list: Formatted data in rich format
            """
            numeric_values = [
                value for value in data if isinstance(value, (int, float))
            ]
            if numeric_values:
                min_value = min(numeric_values)
                max_value = max(numeric_values)
                enable_colors = len(numeric_values) != 1

            formatted_data = []
            for datum in data:
                if isinstance(datum, (int, float)):
                    if colors is not None and enable_colors:
                        if datum == min_value:
                            formatted_data.append(
                                f"[{colors[0]}]{datum:{data_format[arg]['format']}}[/{colors[0]}]"
                            )
                        elif datum == max_value:
                            formatted_data.append(
                                f"[{colors[1]}]{datum:{data_format[arg]['format']}}[/{colors[1]}]"
                            )
                        else:
                            formatted_data.append(
                                f"{datum:{data_format[arg]['format']}}"
                            )
                    else:
                        formatted_data.append(f"{datum:{data_format[arg]['format']}}")
                else:
                    formatted_data.append(str(datum))
            return formatted_data

        if data_format[arg]["highlight"] != Highlight.none:
            if data_format[arg]["highlight"] == Highlight.smallest:
                return functools.partial(format_func, ("green", "red"))
            return functools.partial(format_func, ("red", "green"))
        return functools.partial(format_func, None)

    def finalize(self, *args):
        """
        This should be called prior to printing the benchmark. The arguments to this
        function are the stats that are going to be printed. For these stats,
        it computes the formatted strings and saves them for rich to print later.
        """
        for arg in args:
            format_func = self._data_format(arg)
            self._final_stats[arg] = format_func(self.stats[arg])

    def has_type(self, search_str: str):
        for benchmark_type in self.stats["type"]:
            if search_str.upper() in str(benchmark_type).upper():
                return True
        return False

    def get_row(self):
        """
        Get rows of data for this benchmark based on what was done by finalize()

        Yields:
            list[str]: one row
        """
        for index, _ in enumerate(self.stats["min"]):
            yield [
                str(self._final_stats[arg][index]) for arg in self._final_stats.keys()
            ]


class Benchmarks:
    """
    This class holds all the benchmarks and can be used to print them to console.
    """

    def __init__(self, benchmark, path=None, normalize=False) -> None:
        if normalize:
            normalized_benchmark = self._normalize(benchmark)
            self._benchmark = normalized_benchmark
        else:
            self._benchmark = benchmark
        self.benchmarks = {}
        self.path = path
        for test in benchmark["benchmarks"]:
            self._analyze(test)

    @property
    def machine_info(self):
        return self._benchmark["machine_info"]

    @property
    def commit_info(self):
        return self._benchmark["commit_info"]

    def _analyze(self, test: dict):
        """
        For each benchmark, add a new Benchmark object for it if it's new or add
        it to an existing object

        Args:
            test (dict): The dict defining the benchmark. This is in the
            same format as output from pytest_benchmark.
        """
        group = test["group"]
        if group not in self.benchmarks:
            self.benchmarks[group] = Benchmark(test)
        else:
            self.benchmarks[group].add(test)

    def _normalize(self, benchmarks: dict):
        normalized_benchmarks = []
        for benchmark in benchmarks["benchmarks"]:
            if "load" not in benchmark:
                benchmark["load"] = 1
            if "type" not in benchmark:
                if "type" in benchmark["extra_info"]:
                    benchmark["type"] = benchmark["extra_info"]["type"]
                else:
                    benchmark["type"] = "Unknown"
            if "config" not in benchmark:
                if "config" in benchmark["extra_info"]:
                    benchmark["config"] = benchmark["extra_info"]["config"]
                else:
                    benchmark["config"] = "Unknown"
            if "ops_uncertainty" not in benchmark["stats"]:
                benchmark["stats"]["ops_uncertainty"] = benchmark["stats"]["ops"] - (
                    1 / (benchmark["stats"]["mean"] + benchmark["stats"]["stddev"])
                )
            normalized_benchmarks.append(benchmark)
        benchmarks["benchmarks"] = normalized_benchmarks
        return benchmarks

    def add(self, benchmark):
        self._benchmark["benchmarks"].append(benchmark)
        self._analyze(benchmark)

    def get(self) -> list:
        return self._benchmark["benchmarks"]

    def has_type(self, search_str):
        for _, benchmark in self.benchmarks.items():
            if benchmark.has_type(search_str):
                return True
        return False

    def write(self, path=None):
        if path is None:
            path = self.path
        if path is None:
            print(f"No path specified to write to")
            return
        with open(path, "w") as f:
            json.dump(self._benchmark, f, indent=4)

    def clear(self):
        self.benchmarks = {}
        self._benchmark["benchmarks"] = []

    def print(self):
        console = Console()
        table = Table(
            show_header=True,
            header_style="bold magenta",
            title="Legend",
            title_style="bold yellow",
        )
        table.add_column("Term")
        table.add_column("Meaning", max_width=80)
        show_legend = False
        if self.has_type("wrk"):
            table.add_row(
                "Tx:Cy:tz",
                "Indicates wrk's configuration i.e. that the wrk application was run with x threads, y total TCP connections and for z time",
            )
            show_legend = True
        if self.has_type("cpp"):
            table.add_row(
                "Ix:Ty",
                "Indicates the cpp benchmark's configuration i.e. it was run with x images and y threads",
            )
            show_legend = True
        if show_legend:
            console.print(table)

        for key, value in self.benchmarks.items():
            table = Table(
                show_header=True,
                header_style="bold magenta",
                title=key,
                title_style="bold yellow",
            )
            table.add_column("Name")
            table.add_column("Type")
            table.add_column("Config")
            table.add_column("Workers")
            table.add_column("Min (s)")
            table.add_column("Max (s)")
            table.add_column("Mean (s)")
            table.add_column("StdDev (s)")
            # table.add_column("Median (s)")
            table.add_column("Op/s")
            table.add_column("Op/s +/-")
            value.finalize(
                "name",
                "type",
                "config",
                "load",
                "min",
                "max",
                "mean",
                "stddev",
                "ops",
                "ops_uncertainty",
            )
            for row in value.get_row():
                table.add_row(*row)
            console.print(table)

    def __str__(self):
        return json.dumps(self._benchmark)


def get_last_benchmark_path() -> Optional[str]:
    """
    Get the path to the last generated benchmark in the benchmark directory

    Returns:
        Optional[str]: Path to the benchmark if found
    """
    dir = os.getenv("PROTEUS_ROOT") + "/tests/python/.benchmarks"
    machine_id = pytest_benchmark.utils.get_machine_id()

    list_of_files = pathlib.Path(f"{dir}/{machine_id}").rglob("*.json")
    if list_of_files:
        return max(list_of_files, key=os.path.getctime)
    return None


def get_benchmark(path: Optional[str] = None, normalize=False) -> Optional[Benchmarks]:
    """
    Get the last generated benchmark from the directory

    Returns:
        Optional[dict]: The last benchmark or None if an empty directory is used
    """
    if path is not None:
        last_benchmark = path
    else:
        last_benchmark = get_last_benchmark_path()
    if last_benchmark is not None:
        with open(last_benchmark, "r") as f:
            return Benchmarks(json.load(f), last_benchmark, normalize)
    return None


def parse_wrk_output(output: str) -> dict:
    """
    Parse the terminal output of wrk into a dictionary. The output format is
    defined by write_lua() in the tests.

    Args:
        output (str): Raw terminal output from wrk

    Returns:
        dict: Organized results from wrk
    """
    lines = output.split("\n")
    raw_stats = lines[-4:-1]

    latencies = raw_stats[0].split(",")
    requests = raw_stats[1].split(",")
    summaries = raw_stats[2].split(",")

    stats = {
        "latencies": {},
        "requests": {},
        "summaries": {},
    }

    metrics = ["min", "max", "mean", "stdev"]
    metrics_summary = ["duration", "requests", "bytes"]

    for latency, label in zip(latencies[:2], metrics[:2]):
        stats["latencies"][label] = int(latency)
    for latency, label in zip(latencies[2:], metrics[2:]):
        stats["latencies"][label] = float(latency)

    for request, label in zip(requests[:2], metrics[:2]):
        stats["requests"][label] = int(request)
    for request, label in zip(requests[2:], metrics[2:]):
        stats["requests"][label] = float(request)

    for summary, label in zip(summaries, metrics_summary):
        stats["summaries"][label] = int(summary)

    return stats


BASE_BENCHMARK = {
    "group": "facedetect_dpucadf8h",
    "name": "test_benchmark_facedetect_dpucadf8h_1",
    "fullname": "test_facedetect.py::TestInferImageFacedetectDPUCADF8H::test_benchmark_facedetect_dpucadf8h_1",
    "params": None,
    "param": None,
    "stats": {
        "min": 0.0855791429639794,
        "max": 0.10293014405760914,
        "mean": 0.093361033460083,
        "stddev": 0.005477697831711018,
        "rounds": 11,
        "median": 0.09155373001703992,
        "iqr": 0.005927730540861376,
        "q1": 0.09056435847014654,
        "q3": 0.09649208901100792,
        "iqr_outliers": 0,
        "stddev_outliers": 4,
        "outliers": "4;0",
        "ld15iqr": 0.0855791429639794,
        "hd15iqr": 0.10293014405760914,
        "ops": 10.71110679625837,
        "ops_uncertainty": 10.71110679625837,
        "total": 1.026971368060913,
        "iterations": 1,
    },
}


def make_wrk_benchmarks(raw_stats, benchmark, wrk_config, load):
    wrk_setting = f"T{wrk_config[0]}:C{wrk_config[1]}:t{wrk_config[2]}"

    benchmark_wrk = copy.deepcopy(benchmark)
    # clear other stats from pytest-benchmark
    del benchmark_wrk["extra_info"]
    del benchmark_wrk["options"]
    benchmark_wrk["load"] = load
    benchmark_wrk["type"] = "rest (wrk)"
    benchmark_wrk["config"] = wrk_setting
    benchmark_wrk["name"] = benchmark["name"]
    benchmark_wrk["stats"]["rounds"] = 1
    benchmark_wrk["stats"]["median"] = None
    benchmark_wrk["stats"]["iqr"] = None
    benchmark_wrk["stats"]["q1"] = None
    benchmark_wrk["stats"]["q3"] = None
    benchmark_wrk["stats"]["iqr_outliers"] = None
    benchmark_wrk["stats"]["stddev_outliers"] = None
    benchmark_wrk["stats"]["outliers"] = None
    benchmark_wrk["stats"]["ld15iqr"] = None
    benchmark_wrk["stats"]["hd15iqr"] = None
    benchmark_wrk["stats"]["total"] = None
    benchmark_wrk["stats"]["iterations"] = 1

    # these tables are not currently being used

    # benchmark_latency = copy.deepcopy(benchmark_wrk)
    # benchmark_latency["group"] = benchmark_latency["group"] + " (wrk latencies per thread)"
    # # convert latencies from us to s
    # benchmark_latency["stats"]["min"] = raw_stats["latencies"]["min"] / 1E6
    # benchmark_latency["stats"]["max"] = raw_stats["latencies"]["max"] / 1E6
    # benchmark_latency["stats"]["mean"] = raw_stats["latencies"]["mean"] / 1E6
    # benchmark_latency["stats"]["stddev"] = raw_stats["latencies"]["stdev"] / 1E6
    # benchmark_latency["stats"]["ops"] = 1 / (raw_stats["latencies"]["mean"] / 1E6)

    # benchmark_request = copy.deepcopy(benchmark_wrk)
    # benchmark_request["group"] = benchmark_request["group"] + " (wrk requests/time per thread)"
    # benchmark_request["stats"]["min"] = raw_stats["requests"]["min"]
    # benchmark_request["stats"]["max"] = raw_stats["requests"]["max"]
    # benchmark_request["stats"]["mean"] = raw_stats["requests"]["mean"]
    # benchmark_request["stats"]["stddev"] = raw_stats["requests"]["stdev"]
    # benchmark_request["stats"]["ops"] = raw_stats["requests"]["mean"]

    benchmark_summary = copy.deepcopy(benchmark_wrk)
    benchmark_summary["stats"]["min"] = raw_stats["latencies"]["min"] / 1e6
    benchmark_summary["stats"]["max"] = raw_stats["latencies"]["max"] / 1e6
    benchmark_summary["stats"]["mean"] = raw_stats["latencies"]["mean"] / 1e6
    benchmark_summary["stats"]["stddev"] = raw_stats["latencies"]["stdev"] / 1e6
    benchmark_summary["stats"]["ops"] = raw_stats["summaries"]["requests"] / (
        raw_stats["summaries"]["duration"] / 1e6
    )

    # wrk's stddev and means are per thread which doesn't correlate directly to the mean
    # so calculating uncertainty with them is not accurate
    benchmark_summary["stats"]["ops_uncertainty"] = "N/A"

    return (
        # (benchmark_latency, "wrk latencies per thread"),
        # (benchmark_request, "wrk requests/time per thread"),
        benchmark_summary,
    )


def combine_wrk_stats(samples):
    stats = {
        "latencies": {},
        "requests": {},
        "summaries": {},
    }

    stats["latencies"]["min"] = min([x["latencies"]["min"] for x in samples])
    stats["latencies"]["max"] = max([x["latencies"]["max"] for x in samples])
    stats["latencies"]["mean"] = statistics.mean(
        [x["latencies"]["mean"] for x in samples]
    )
    # pooled std dev
    stats["latencies"]["stdev"] = math.sqrt(
        sum([math.pow(x["latencies"]["stdev"], 2) for x in samples])
    )
    stats["summaries"]["requests"] = sum([x["summaries"]["requests"] for x in samples])
    stats["summaries"]["duration"] = sum([x["summaries"]["duration"] for x in samples])

    return stats


def wrk_benchmarks(config: Config, benchmarks: Benchmarks):
    client = Client("127.0.0.1:8998")
    server = Server()
    wrk_options = config.wrk
    with Progress() as progress:
        task0 = progress.add_task(
            "Running wrk benchmarks...", total=len(benchmarks.get())
        )
        for benchmark in benchmarks.get():
            try:
                extra_info = benchmark["extra_info"]
            except KeyError:
                progress.update(task0, advance=1)
                continue
            if "lua" in extra_info:
                lua_file = (
                    os.getenv("PROTEUS_ROOT") + f"/tests/python/{extra_info['lua']}.lua"
                )
                if not os.path.exists(lua_file):
                    print(f"Lua file not found, skipping: {lua_file}")
                    continue

                if wrk_options.workers is not None:
                    loads = wrk_options.workers
                else:
                    loads = [1]

                server.start(True)
                client.wait_until_live()

                repeat_wrk_count = config.repeat
                task1 = progress.add_task(
                    f"Running {benchmark['name']}", total=len(loads)
                )
                for load in loads:
                    model = extra_info["model"]
                    parameters = extra_info["parameters"]
                    if parameters is None:
                        parameters = {"share": False}
                    else:
                        parameters["share"] = False
                    for _ in range(load):
                        client.load(model, parameters)
                    infer_endpoint = client.get_address("infer", model)
                    # print(f"Loading {load} copies of the {model} model")
                    total = (
                        len(wrk_options.threads)
                        * len(wrk_options.connections)
                        * len(wrk_options.time)
                    )
                    task2 = progress.add_task(
                        f"Running with {load} worker(s)...", total=total
                    )
                    for wrk_config in itertools.product(
                        wrk_options.threads, wrk_options.connections, wrk_options.time
                    ):
                        # TODO(varunsh): we need to check how many requests actually succeeded
                        wrk_command = [
                            "wrk",
                            f"-t{wrk_config[0]}",
                            f"-c{wrk_config[1]}",
                            f"-d{wrk_config[2]}",
                            "-s",
                            lua_file,
                            infer_endpoint,
                        ]
                        if config.verbosity > 0:
                            print(f"wrk command: \n  {' '.join(wrk_command)}")
                        wrk_stats = []
                        task3 = progress.add_task(
                            f"Running wrk: threads: {wrk_config[0]}, TCP connections: {wrk_config[1]}, time: {wrk_config[2]}",
                            total=repeat_wrk_count,
                        )
                        for _ in range(repeat_wrk_count):
                            ret = subprocess.run(
                                wrk_command, check=True, stdout=subprocess.PIPE
                            )

                            wrk_output = ret.stdout.decode("utf-8")
                            wrk_stats.append(parse_wrk_output(wrk_output))
                            progress.update(task3, advance=1)
                        raw_stats = combine_wrk_stats(wrk_stats)
                        wrk_benchmarks = make_wrk_benchmarks(
                            raw_stats, benchmark, wrk_config, load
                        )
                        for wrk_benchmark in wrk_benchmarks:
                            benchmarks.add(wrk_benchmark)
                        progress.update(task2, advance=1)
                    for _ in range(load):
                        client.unload(model)

                    progress.update(task1, advance=1)

                server.stop()
                client.wait_until_stop()
            progress.update(task0, advance=1)

    return benchmarks


def parse_cpp_output(output: str) -> dict:
    """
    Parse the terminal output of cpp tests into a dictionary

    Args:
        output (str): Raw terminal output from the executable

    Returns:
        dict: Organized results from the executable
    """
    lines = output.split("\n")
    raw_stats = lines[-3:-1]

    queries = raw_stats[0].split(" ")[4]
    time = raw_stats[0].split(" ")[6]
    queries_per_sec = raw_stats[1].split(" ")[4]

    stats = {
        "queries": int(queries),
        "time": float(time) / 1000,  # convert to seconds
        "qps": float(queries_per_sec),
    }

    return stats


def combine_cpp_stats(samples):
    stats = {}

    stats["min"] = min([x["time"] / x["queries"] for x in samples])
    stats["max"] = max([x["time"] / x["queries"] for x in samples])
    stats["mean"] = statistics.mean([x["time"] / x["queries"] for x in samples])
    if len(samples) > 1:
        stats["stdev"] = statistics.stdev(
            [x["time"] / x["queries"] for x in samples], stats["mean"]
        )
    else:
        stats["stdev"] = 0
    stats["ops"] = statistics.mean([x["qps"] for x in samples])

    return stats


def make_cpp_benchmarks(raw_stats, path: pathlib.Path, cpp_config, repeat):
    cpp_setting = f"I{cpp_config[0]}:T{cpp_config[1]}"

    name = path.stem

    benchmark = copy.deepcopy(BASE_BENCHMARK)
    if name.startswith("test_"):
        benchmark["group"] = name[len("test_") :]
    else:
        benchmark["group"] = name
    benchmark["type"] = "native (cpp)"
    benchmark["config"] = cpp_setting
    benchmark["fullname"] = str(path)
    benchmark["load"] = cpp_config[2]
    benchmark["name"] = name
    if cpp_config[3]:
        benchmark["name"] += f" (reference)"
    benchmark["stats"]["rounds"] = 1
    benchmark["stats"]["median"] = None
    benchmark["stats"]["iqr"] = None
    benchmark["stats"]["q1"] = None
    benchmark["stats"]["q3"] = None
    benchmark["stats"]["iqr_outliers"] = None
    benchmark["stats"]["stddev_outliers"] = None
    benchmark["stats"]["outliers"] = None
    benchmark["stats"]["ld15iqr"] = None
    benchmark["stats"]["hd15iqr"] = None
    benchmark["stats"]["total"] = None
    benchmark["stats"]["iterations"] = repeat

    benchmark["stats"]["min"] = raw_stats["min"]
    benchmark["stats"]["max"] = raw_stats["max"]
    benchmark["stats"]["mean"] = raw_stats["mean"]
    benchmark["stats"]["stddev"] = raw_stats["stdev"]
    benchmark["stats"]["ops"] = raw_stats["ops"]
    mean = benchmark["stats"]["mean"]
    stddev = benchmark["stats"]["stddev"]
    ops = benchmark["stats"]["ops"]
    benchmark["stats"]["ops_uncertainty"] = ops - (1 / (mean + stddev))

    return benchmark


def get_benchmark_exe(path: pathlib.Path):
    relative_path_to_exe = (
        str(path.parent)[len(os.getenv("PROTEUS_ROOT")) :] + f"/{path.stem}"
    )
    benchmark_path = os.getenv("PROTEUS_ROOT") + f"/build/Release{relative_path_to_exe}"
    if not os.path.exists(benchmark_path):
        return None
    with open(path, "r") as f:
        for line in f:
            if "@brief Benchmark" in line:
                return benchmark_path
            if line.strip().startswith("#include"):
                return None
    return None


def cpp_benchmarks(config: Config, benchmarks: Benchmarks):
    benchmarks_to_run = set()
    benchmark_dir = os.getenv("PROTEUS_ROOT") + "/tests/cpp"
    accept_all_benchmarks = True if config.benchmarks is None else False
    for path in pathlib.Path(benchmark_dir).rglob("*.cpp"):
        if not accept_all_benchmarks:
            expression = Expression.compile(config.benchmarks)
            if not expression.evaluate(KeywordMatcher([path.stem])):
                continue

        benchmark = get_benchmark_exe(path)
        if benchmark is not None:
            benchmarks_to_run.add(benchmark)

    cpp_options = config.cpp
    repeat_count = config.repeat

    with Progress() as progress:
        task0 = progress.add_task(
            f"Running cpp benchmarks...", total=len(benchmarks_to_run)
        )
        for benchmark in benchmarks_to_run:
            valid_flags = [""]
            for flag in cpp_options.flags:
                if not flag:
                    continue
                cpp_command = [benchmark, "--help"]
                ret = subprocess.run(cpp_command, check=True, stdout=subprocess.PIPE)
                help_output = ret.stdout.decode("utf-8")

                if help_output.find(flag.split(" ")[0]) != -1:
                    valid_flags.append(flag)

            total = (
                len(cpp_options.images)
                * len(cpp_options.threads)
                * len(cpp_options.workers * len(valid_flags))
            )
            task1 = progress.add_task(f"Running {benchmark}", total=total)
            for cpp_config in itertools.product(
                cpp_options.images,
                cpp_options.threads,
                cpp_options.workers,
                valid_flags,
            ):
                cpp_command = [
                    benchmark,
                    "-i",
                    str(cpp_config[0]),
                    "-t",
                    str(cpp_config[1]),
                    "-r",
                    str(cpp_config[2]),
                    cpp_config[3],
                ]
                cpp_stats = []
                task2 = progress.add_task(
                    f"Configuration: Images: {cpp_config[0]}, Threads: {cpp_config[1]}, Workers: {cpp_config[2]}, Flags: {cpp_config[3]}",
                    total=repeat_count,
                )
                if config.verbosity > 0:
                    print(f"cpp command: \n  {' '.join(cpp_command)}")
                for _ in range(repeat_count):
                    ret = subprocess.run(
                        cpp_command, check=True, stdout=subprocess.PIPE
                    )
                    cpp_output = ret.stdout.decode("utf-8")
                    # print(cpp_output)
                    cpp_stats.append(parse_cpp_output(cpp_output))
                    progress.update(task2, advance=1)
                raw_stats = combine_cpp_stats(cpp_stats)
                cpp_benchmark = make_cpp_benchmarks(
                    raw_stats, pathlib.Path(benchmark), cpp_config, repeat_count
                )
                benchmarks.add(cpp_benchmark)
                progress.update(task1, advance=1)
            progress.update(task0, advance=1)

    return benchmarks


def pytest_benchmarks(config: Config, quiet=False):
    def task():
        subprocess.run(["/bin/bash", "-c", cmd], stdout=subprocess.PIPE, check=True)

    cmd = os.getenv("PROTEUS_ROOT") + "/tests/test.sh --benchmark only"
    if config.benchmarks:
        cmd += f' -k "{config.benchmarks}"'
    if not quiet:
        with Progress() as progress:
            task1 = progress.add_task(f"Running pytest benchmarks...", total=1)
            task()
            progress.update(task1, advance=1)
    else:
        task()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run Proteus benchmarking")
    parser.add_argument(
        "-k",
        action="store",
        default="",
        help="choose which benchmarks to run by substring",
    )
    parser.add_argument(
        "--print",
        action="store_true",
        default=False,
        help="print the last benchmark and exit",
    )
    args = parser.parse_args()

    if args.print:
        benchmarks = get_benchmark()
        benchmarks.print()
        sys.exit(0)

    config = Config(os.getenv("PROTEUS_ROOT") + "/tools/benchmark.yml")
    if args.k:
        config.benchmarks = args.k

    if config.pytest.enabled:
        pytest_benchmarks(config)
    else:
        # if pytest is disabled, run anyway with a small test suite to create a
        # new benchmark file that can be populated by later benchmarks
        tmp = config.benchmarks
        config.benchmarks = "echo"
        pytest_benchmarks(config, True)
        config.benchmarks = tmp
        benchmarks = get_benchmark(normalize=True)
        benchmarks.clear()
        benchmarks.write()

    # normalize the pytest generated benchmarks to be in line with the others
    benchmarks = get_benchmark(normalize=True)

    if config.wrk.enabled:
        benchmarks = wrk_benchmarks(config, benchmarks)

    if config.cpp.enabled:
        benchmarks = cpp_benchmarks(config, benchmarks)

    benchmarks.write()
    benchmarks.print()
