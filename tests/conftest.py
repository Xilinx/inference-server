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
from helper import build_path, kDefaultHttpPort, root_path, run_path
from pytest_cpp.plugin import CppItem
from xprocess import ProcessStarter

import proteus
import proteus.clients
import proteus.servers

proteus_command = []
http_server_addr = ""


@pytest.hookimpl
def pytest_sessionstart(session):
    global proteus_command
    global http_server_addr

    proteus_command = [str(run_path)]
    http_port = session.config.getoption("--http_port")
    proteus_command.extend(["--http-port", str(http_port)])
    proteus_command.extend(["--model-repository", root_path / "external/repository"])

    http_server_addr = "http://" + get_http_addr(session.config)
    addr = socket.gethostbyname(session.config.getoption("hostname"))
    ip_addr = ipaddress.ip_address(addr)
    if not ip_addr.is_loopback:
        client = proteus.clients.HttpClient(http_server_addr)
        if not client.serverLive():
            pytest.exit(f"No HTTP server found at {http_server_addr}", returncode=1)


@pytest.hookimpl
def pytest_addoption(parser):
    parser.addoption("--hostname", action="store", default="127.0.0.1")
    parser.addoption("--http_port", action="store", default=kDefaultHttpPort)
    parser.addoption("--fpgas", action="store", default="")
    parser.addoption("--benchmark", action="store", default="skip")
    parser.addoption("--perf", action="store", default="skip")
    parser.addoption("--cpp", action="store", default="all")

    # TODO(varunsh): this is currently not exposed via the test runner script
    parser.addoption("--skip-extensions", nargs="+", default=[])


def get_http_addr(config):
    """
    Get the address of the Proteus server

    Args:
        config (Config): Pytest config object containing the options

    Returns:
        str: Address of the Proteus server
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
            if cpp_mode == "only":
                skip_bench = pytest.mark.skip(
                    reason=f"Not a cpp test: use --cpp [all, skip] to run"
                )
                item.add_marker(skip_bench)
            continue
        if cpp_mode == "skip":
            skip_bench = pytest.mark.skip(
                reason=f"A cpp test: use --cpp [all, only] to run"
            )
            item.add_marker(skip_bench)
            continue
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
    global http_server_addr
    global proteus_command

    add_cpp_markers(items, config.getoption("--cpp"))

    client = proteus.clients.HttpClient(http_server_addr)
    if not client.serverLive():
        server = proteus.servers.Server()
        server.startHttp(8998)
        client = proteus.clients.HttpClient("http://127.0.0.1:8998")
        while not client.serverLive():
            time.sleep(1)

    filter_tests(items, config.getoption("--benchmark"), "benchmark")
    filter_tests(items, config.getoption("--perf"), "perf")

    fpgas_option = str(config.getoption("--fpgas"))
    if fpgas_option:
        fpga_arg: str = fpgas_option
        try:
            # parse --fpga options from DPUCADF8H:1,DPUCAHX8H:2... to {"DPUCADF8H": 1, "DPUCAHX8H": 2, ...}
            fpgas_avail = dict(
                (fpga[0], int(fpga[1]))
                for fpga in [x.split(":") for x in fpga_arg.split(",")]
            )
        except:
            fpgas_avail = {}
    else:
        fpgas_avail = {}

    client = proteus.clients.HttpClient(http_server_addr)
    for item in items:
        for mark in item.iter_markers(name="fpgas"):
            fpga_i = mark.args[0]
            fpga_num_i = int(mark.args[1])

            if fpga_i not in fpgas_avail or fpga_num_i > fpgas_avail[fpga_i]:
                if client.hasHardware(fpga_i, fpga_num_i):
                    fpgas_avail[fpga_i] = fpga_num_i
                if fpga_i not in fpgas_avail or fpga_num_i > fpgas_avail[fpga_i]:
                    skip_fpga = pytest.mark.skip(
                        reason=f"Needs {fpga_num_i} {fpga_i} FPGA(s). Use --fpgas {fpga_i}:{fpga_num_i} to specify"
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
def server(xprocess):
    global http_server_addr
    global proteus_command

    client = proteus.clients.HttpClient(http_server_addr)
    if not client.serverLive():

        class Starter(ProcessStarter):
            pattern = "HTTP server starting at port"

            terminate_on_interrupt = True

            def startup_check(self):
                while not client.serverLive():
                    time.sleep(1)
                return True

            args = proteus_command

        xprocess.ensure("server", Starter)

        yield

        xprocess.getinfo("server").terminate()

        while client.serverLive():
            time.sleep(1)
    else:
        yield


@pytest.fixture(scope="class")
def load(request, rest_client, model_fixture, parameters_fixture: dict, server):
    parameters = proteus.RequestParameters()
    if parameters_fixture is not None:
        for key, value in parameters_fixture.items():
            parameters.put(key, value)

    response = rest_client.workerLoad(model_fixture, parameters)
    request.cls.model = response

    while not rest_client.modelReady(response):
        time.sleep(1)

    yield  # perform testing

    rest_client.modelUnload(response)


@pytest.fixture(scope="class")
def rest_client(request):
    address = get_http_addr(request.config)
    return proteus.clients.HttpClient("http://" + address)


@pytest.fixture(scope="class")
def ws_client(request):
    address = get_http_addr(request.config)
    return proteus.clients.WebSocketClient("ws://" + address, "http://" + address)


@pytest.fixture(autouse=True, scope="class")
def assign_client(request, rest_client, ws_client):
    request.cls.rest_client = rest_client
    request.cls.ws_client = ws_client
