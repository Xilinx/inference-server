# Copyright 2021 Xilinx, Inc.
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

import ipaddress
import os
import re
import socket
import subprocess
import time

import pytest
import yaml
from pytest_cpp.plugin import CppItem
from xprocess import ProcessStarter

import amdinfer

from helper import build_path, kDefaultHttpPort, root_path, run_path


@pytest.hookimpl
def pytest_addoption(parser):
    parser.addoption("--hostname", action="store", default="127.0.0.1")
    parser.addoption("--http-port", action="store", default=kDefaultHttpPort)
    parser.addoption("--runtime-config", action="store", default="./tests/conf.yaml")

    # TODO(varunsh): this is currently not exposed via the test runner script
    parser.addoption("--skip-extensions", nargs="+", default=[])


def get_http_addr(config):
    """
    Get the address of the server

    Args:
        config (Config): Pytest config object containing the options

    Returns:
        str: Address of the server
    """
    hostname = config.getoption("hostname")
    http_port = config.getoption("http_port")
    return f"{hostname}:{http_port}"


def add_cpp_markers(items, cpp_mode):
    """
    For all the collected items, if the item is a C++ test, find its
    corresponding source file (assuming it has a matching name). If found,
    extract all the pytest decorations in comments and add them to the test

    Args:
        items (list): list of Nodes from pytest_collection_modifyitems
        cpp_mode (str): skip | all | only
    """
    for item in items:
        if not isinstance(item, CppItem):
            continue
        item.add_marker(pytest.mark.cpp())
        test_dir = str(item.fspath).replace(str(build_path), str(root_path))
        # this test naming syntax is defined in cmake/AddTest.cmake
        test_file = "test_" + item.fspath.basename.split("-")[1]
        source_path = test_dir.replace(item.fspath.basename, test_file) + ".cpp"
        if not os.path.exists(source_path):
            continue
        with open(source_path) as f:
            lines = f.read()
        # type parameterized tests have x/name.test/# format
        base_name = item.name.split("/")
        test_name = base_name[1] if len(base_name) > 1 else base_name[0]
        test = test_name.split(".")
        bar = f"((\/\/ @pytest.*\s)*)(^\/\/.*\s)?^TEST[_]?[FP]?\({test[0]}, {test[1]}\)"
        match = re.search(bar, lines, re.MULTILINE)
        if not match:
            continue
        marks = match.group(1).split("\n")
        for mark in marks:
            if not mark:
                continue
            try:
                # 4: to remove the leading "// @"
                my_mark = eval(mark[4:])
                item.add_marker(my_mark)
            except Exception:
                pass


def filter_tests(items, mode, label):
    if mode == "only":
        skip_bench = pytest.mark.skip(
            reason=f"Not a {label} test: use --{label} [all, skip] to run"
        )
        for item in items:
            if label not in item.keywords:
                item.add_marker(skip_bench)
    elif mode == "all":
        pass
    else:
        skip_bench = pytest.mark.skip(
            reason=f"{label} test: use --{label} [all, only] to run"
        )
        for item in items:
            if label in item.keywords:
                item.add_marker(skip_bench)


def pytest_collection_modifyitems(config, items):
    http_address = get_http_addr(config)
    http_server_addr = "http://" + http_address

    # add_cpp_markers(items, config.getoption("--cpp"))
    add_cpp_markers(items, "all")

    client = amdinfer.HttpClient(http_server_addr)
    if not client.serverLive():
        server = amdinfer.Server()
        server.startHttp(8998)
        client = amdinfer.HttpClient("http://127.0.0.1:8998")
        while not client.serverLive():
            time.sleep(1)

    fpgas_avail = {}

    client = amdinfer.HttpClient(http_server_addr)
    for item in items:
        for mark in item.iter_markers(name="fpgas"):
            fpga_i = mark.args[0]
            fpga_num_i = int(mark.args[1])

            if fpga_i not in fpgas_avail or fpga_num_i > fpgas_avail[fpga_i]:
                if client.hasHardware(fpga_i, fpga_num_i):
                    fpgas_avail[fpga_i] = fpga_num_i
                if fpga_i not in fpgas_avail or fpga_num_i > fpgas_avail[fpga_i]:
                    skip_fpga = pytest.mark.skip(
                        reason=f"Needs {fpga_num_i} {fpga_i} FPGA(s) but not found on the server"
                    )
                    item.add_marker(skip_fpga)

    metadata = client.serverMetadata()
    extensions = set(extension for extension in metadata.extensions)

    for extension in config.getoption("--skip-extensions"):
        if extension in extensions:
            del extensions[extension]

    for item in items:
        for mark in item.iter_markers(name="extensions"):
            requested_extensions = set(mark.args[0])

            if not requested_extensions.issubset(extensions):
                skip_extension = pytest.mark.skip(
                    reason=f"Needs {requested_extensions} support from the server. Server provides {extensions}."
                )
                item.add_marker(skip_extension)


@pytest.fixture(scope="class")
def server(xprocess, request):
    address = get_http_addr(request.config)
    client = amdinfer.HttpClient("http://" + address)
    try:
        ready = client.serverReady()
    except amdinfer.ConnectionError:
        ready = False

    if not ready:

        class Starter(ProcessStarter):
            pattern = "HTTP server starting at port"

            terminate_on_interrupt = True

            def startup_check(self):
                amdinfer.waitUntilServerReady(client)
                return True

            amdinfer_command = [str(run_path)]
            http_port = request.config.getoption("--http-port")
            amdinfer_command.extend(["--http-port", str(http_port)])
            amdinfer_command.extend(
                ["--model-repository", root_path / "external/artifacts/repository"]
            )
            args = amdinfer_command

        xprocess.ensure("server", Starter)

        yield

        xprocess.getinfo("server").terminate()

        while client.serverLive():
            time.sleep(1)
    else:
        yield


@pytest.fixture(scope="class")
def load(request, server):
    test_model, test_parameters = request.cls.get_config()

    assert test_model

    parameters = amdinfer.ParameterMap()
    if test_parameters is not None:
        for key, value in test_parameters.items():
            parameters.put(key, value)

    request.cls.rest_client = rest_client(request)
    request.cls.ws_client = ws_client(request)

    response = request.cls.rest_client.workerLoad(test_model, parameters)
    request.cls.endpoint = response

    while not request.cls.rest_client.modelReady(response):
        time.sleep(1)

    yield  # perform testing

    request.cls.rest_client.modelUnload(response)

    while request.cls.rest_client.modelReady(response):
        time.sleep(1)

    request.cls.ws_client = None
    request.cls.rest_client = None


def rest_client(request):
    address = get_http_addr(request.config)
    return amdinfer.HttpClient("http://" + address)


def ws_client(request):
    address = get_http_addr(request.config)
    return amdinfer.WebSocketClient("ws://" + address, "http://" + address)


# we can eventually parameterize the clients with a fixture like this
# @pytest.fixture(scope="class")
# def client(request):
#     http_address = get_http_addr(request.config)
#     if request.param == "http":
#         http_client = amdinfer.HttpClient("http://" + http_address)
#         if not http_client.serverLive():
#             pytest.skip("HTTP client could not connect to server")
#         return http_client
#     elif request.param == "websocket":
#         websocket_client = amdinfer.WebSocketClient("ws://" + http_address, "http://" + http_address)
#         if not websocket_client.serverLive():
#             pytest.skip("Websocket client could not connect to server")
#         return websocket_client
#     else:
#         raise ValueError(f"Unknown value for client: {request.param}")


@pytest.fixture(scope="class")
def assign_client(request):
    request.cls.rest_client = rest_client(request)
    request.cls.ws_client = ws_client(request)

    yield  # perform testing

    request.cls.ws_client = None
    request.cls.rest_client = None


@pytest.fixture(scope="session")
def runtime_config(request):
    config_file_path = request.config.getoption("runtime_config")
    with open(config_file_path, "r") as f:
        config = yaml.safe_load(f)
        assert isinstance(config, dict)
    return config
