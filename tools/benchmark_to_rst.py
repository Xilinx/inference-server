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


def endpoint_to_name(endpoint):
    if endpoint == "xmodel":
        return "Vitis AI"
    raise ValueError(f"Unknown endpoint: {endpoint}")


def get_header(benchmark):
    row = ["Model", "Backend"]

    labels = benchmark["label"].split("/")
    for label in labels:
        text, value = label.split(":")
        text = text.replace("_", " ")
        text = " ".join([x.capitalize() for x in text.split()])
        row.append(text)

    time_unit = benchmark["time_unit"]
    row.append(f"Time ({time_unit})")

    return ",".join(row)


def parse_benchmark(benchmark):
    name = benchmark["name"].split("/")
    model = name[0]
    backend = endpoint_to_name(name[1])
    config = name[2:]

    time = benchmark["real_time"]

    labels = benchmark["label"].split("/")
    assert len(labels) == len(config)

    row = [
        model,
        backend,
    ]

    for label in labels:
        text, value = label.split(":")
        # get the actual value not the requested value
        row.append(value.split("(", 1)[0])

    return ",".join(row), str(round(time, 2))


def parse_benchmarks(json_data):
    benchmarks = json_data["benchmarks"]

    header = get_header(benchmarks[0])

    rows = [header]
    # use a dictionary to remove any duplicated rows. In case of duplicates, the
    # last one is used
    data = {}
    for benchmark in benchmarks:
        key, value = parse_benchmark(benchmark)
        data[key] = value

    for key, value in data.items():
        rows.append(key + "," + value)

    return rows


def main(args: argparse.Namespace):
    with open(args.file, "r") as f:
        json_data = json.load(f)

    rows = parse_benchmarks(json_data)

    print("\n".join(rows))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert the JSON output from Google Benchmark to an RST table"
    )
    parser.add_argument(
        "-f",
        "--file",
        action="store",
        default="",
        help="path to the JSON benchmark output",
    )
    args = parser.parse_args()

    main(args)
