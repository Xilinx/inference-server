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
import glob
import os
import pathlib
import subprocess

import pytest


def get_root():
    root = os.getenv("PROTEUS_ROOT")
    assert root is not None, "PROTEUS_ROOT must be defined in the environment"
    return pathlib.Path(root)


def get_default_markers(args, unknown_args):
    # if a marker filter isn't manually set, set a default filter
    marker_filter = ""
    marker_flags = set(["-m", "--markers"])
    intersection = marker_flags & set(unknown_args)
    if not intersection:
        for flag in ["benchmark", "perf", "cpp"]:
            attr = getattr(args, flag)
            if attr in ["skip", "only"] and marker_filter:
                marker_filter += " and "
            if attr == "skip":
                marker_filter += f"(not {flag})"
            elif attr == "only":
                marker_filter += flag

    return marker_filter


def run_tests(args: argparse.Namespace, unknown_args: list):
    root = get_root()
    build_config_file = root / "build/config.txt"
    try:
        with open(build_config_file, "r") as f:
            args.build = f.read()
    except FileNotFoundError:
        pass

    pytest_args = [
        str(root / "build" / args.build / "tests"),
        str(root / "tests"),
        "-ra",
        "--tb=short",
        "--http-port",
        str(args.http_port),
        "--hostname",
        args.hostname,
    ]

    marker_filter = get_default_markers(args, unknown_args)
    if marker_filter:
        unknown_args.extend(["-m", marker_filter])

    pytest_args.extend(unknown_args)
    if args.dry_run:
        print("Note: some statements may need to be quoted to use this command")
        print("pytest " + " ".join(pytest_args))
    else:
        pytest.main(pytest_args)


def run_examples(args: argparse.Namespace, unknown_args: list):
    root = get_root()

    files = glob.iglob(str(root / "examples/**/*.py"), recursive=True)
    for file in files:
        print(f"Running {file}")
        if not args.dry_run:
            subprocess.check_output(["python3", file])

    files = glob.iglob(
        str(root / "build" / args.build / "examples/**/*"), recursive=True
    )
    for file in files:
        if os.path.isfile(file) and os.access(file, os.X_OK):
            print(f"Running {file}")
            if not args.dry_run:
                subprocess.check_output([file])


def main(args: argparse.Namespace, unknown_args: list):
    # strip any quotes around arguments
    unknown_args = [x.strip('"') for x in unknown_args]

    if args.mode in ["tests", "all"]:
        run_tests(args, unknown_args)
    if args.mode in ["examples", "all"]:
        run_examples(args, unknown_args)


def get_parser(parser=None):
    if parser is None:
        parser = argparse.ArgumentParser(
            prog="test",
            description="Run tests and examples",
            add_help=False,
        )

    command_group = parser.add_argument_group("Options")
    command_group.add_argument(
        "--mode",
        action="store",
        choices={"tests", "examples", "all"},
        default="tests",
        help="Type of tests to run. Defaults to tests",
    )
    command_group.add_argument(
        "--hostname",
        action="store",
        default="127.0.0.1",
        help="hostname of the server. Defaults to 127.0.0.1",
    )
    command_group.add_argument(
        "--http-port",
        action="store",
        default=8998,
        help="port to use for the HTTP server. Defaults to 8998",
    )
    command_group.add_argument(
        "--load-before",
        action="store_true",
        help="attempt to load xclbins to all unloaded FPGAs before running tests. Defaults to false",
    )
    command_group.add_argument(
        "--build",
        action="store",
        default="Debug",
        help="build to test. Defaults to Debug",
    )
    command_group.add_argument(
        "--benchmark",
        action="store",
        default="skip",
        choices={"skip", "only", "all"},
        help="Run benchmark tests. Defaults to skip. Ignored if -m is specified",
    )
    command_group.add_argument(
        "--perf",
        action="store",
        default="skip",
        choices={"skip", "only", "all"},
        help="Run perf tests. Defaults to skip. Ignored if -m is specified",
    )
    command_group.add_argument(
        "--cpp",
        action="store",
        default="all",
        choices={"skip", "only", "all"},
        help="Run cpp tests. Defaults to all. Ignored if -m is specified",
    )

    command_group.add_argument(
        "-h", "--help", action="help", help="show this help message and exit"
    )

    return parser


if __name__ == "__main__":
    parser = get_parser()
    args, unknown_args = parser.parse_known_args()
    main(args, unknown_args)
