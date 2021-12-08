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

import signal
import subprocess
import time

import pytest
import requests

from proteus.rest import Client as RestClient
from proteus.websocket import Client as WebsocketClient
from helper import kDefaultHttpPort, run_path

proteus_command = []
server_addr = ""
pid = 0

# The timeout code is taken from https://stackoverflow.com/a/46773292
class Termination(SystemExit):
    pass


class TimeoutExit(BaseException):
    pass


def _terminate(signum, frame):
    raise Termination("Runner is terminated from outside.")


def isUp(server_addr):
    try:
        requests.get(f"{server_addr}/v2/health/live")
        requests.get(f"{server_addr}/v2/health/ready")
    except:
        return False
    else:
        requests.get(f"{server_addr}/v2")
        return True


def _timeout(signum, frame):
    global proteus_command
    global server_addr
    global pid

    if pid != 0:
        print("\nRestarting Proteus")
        subprocess.call(["kill", "-9", str(pid)])
        p = subprocess.Popen(proteus_command)
        pid = p.pid
        while not isUp(server_addr):
            time.sleep(1)

    # for some reason, without trying to print (which raises an exception),
    # the timeout isn't raised and the test succeeds despite timing out
    # Update: different behavior seen on another machine but left here in case
    try:
        print("Raising timeout exception")
    except KeyboardInterrupt:
        print("Raised keyboard exception")
    raise TimeoutExit()


@pytest.hookimpl
def pytest_sessionstart(session):
    global proteus_command
    global server_addr
    global pid

    server_addr = "http://" + get_server_addr(session.config)
    if not isUp(server_addr):
        print("\nStarting Proteus")

        proteus_command = [run_path]
        http_port = session.config.getoption("--http_port")
        proteus_command.extend(["--http_port", str(http_port)])
        p = subprocess.Popen(proteus_command)
        pid = p.pid
        while not isUp(server_addr):
            time.sleep(1)
    else:
        print("Not starting Proteus as it's already running")


@pytest.hookimpl
def pytest_sessionfinish(session, exitstatus):
    global proteus_command
    global server_addr
    global pid

    if pid != 0:
        print("\nEnding Proteus at pid " + str(pid))
        subprocess.call(["kill", "-2", str(pid)])
        while isUp(server_addr):
            time.sleep(1)


@pytest.hookimpl
def pytest_addoption(parser):
    parser.addoption("--hostname", action="store", default="localhost")
    parser.addoption("--http_port", action="store", default=kDefaultHttpPort)
    parser.addoption("--fpgas", action="store", default="")
    parser.addoption("--benchmark", action="store", default="skip")

    # TODO(varunsh): this is currently not exposed via the test runner script
    parser.addoption("--skip-extensions", nargs="+", default=[])


def get_server_addr(config):
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


@pytest.hookimpl
def pytest_configure(config):
    # register an additional marker
    config.addinivalue_line(
        "markers", "timeout(time): add a timeout value for this test"
    )

    config.addinivalue_line(
        "markers",
        "fpgas(name, num): indicates type and minimum number of FPGAs needed for this test",
    )

    config.addinivalue_line(
        "markers", "benchmark: indicates a benchmark test with logging output"
    )

    config.addinivalue_line(
        "markers", "extensions([names]): indicates a test that uses extensions"
    )

    # Install the signal handlers that we want to process.
    signal.signal(signal.SIGTERM, _terminate)
    signal.signal(signal.SIGALRM, _timeout)


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_protocol(item, nextitem):
    # putting this in runtest_setup or call suppresses the print until the end
    # tw = py.io.TerminalWriter(sys.stdout)
    # tw.line()
    # tw.sep("=", "starting test", bold=True)

    marker = item.get_closest_marker("timeout")
    if marker:
        signal.alarm(marker.args[0])

    try:
        # Run the setup, test body, and teardown stages.
        yield
    finally:
        # Disable the alarm when the test passes or fails.
        # I.e. when we get into the framework's body.
        signal.alarm(0)


def pytest_collection_modifyitems(config, items):
    benchmark_mode = config.getoption("--benchmark")
    skip_bench = pytest.mark.skip(reason="use --benchmark option to run")
    if benchmark_mode == "only":
        for item in items:
            if "benchmark" not in item.keywords:
                item.add_marker(skip_bench)
    elif benchmark_mode == "all":
        pass
    else:
        for item in items:
            if "benchmark" in item.keywords:
                item.add_marker(skip_bench)

    server_addr = "http://" + get_server_addr(config)

    fpgas_option = str(config.getoption("--fpgas"))
    if fpgas_option:
        fpga_arg: str = fpgas_option
    else:
        endpoint = server_addr + "/v2/hardware"
        response = requests.get(endpoint)
        fpga_arg = response.content.decode("utf-8")
    try:
        # parse --fpga options from DPUCADF8H:1,DPUCAHX8H:2... to {"DPUCADF8H": 1, "DPUCAHX8H": 2, ...}
        fpgas_avail = dict(
            (fpga[0], int(fpga[1]))
            for fpga in [x.split(":") for x in fpga_arg.split(",")]
        )
    except:
        fpgas_avail = {}
    for item in items:
        for mark in item.iter_markers(name="fpgas"):
            fpga_i = mark.args[0]
            fpga_num_i = int(mark.args[1])

            if fpga_i not in fpgas_avail or fpga_num_i > fpgas_avail[fpga_i]:
                skip_fpga = pytest.mark.skip(
                    reason=f"Needs {fpga_num_i} {fpga_i} FPGA(s). Use --fpgas {fpga_i}:{fpga_num_i} to specify"
                )
                item.add_marker(skip_fpga)

    endpoint = server_addr + "/v2"
    response = requests.get(endpoint).json()
    extensions = set(extension for extension in response["extensions"])

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
def load(request, rest_client, model_fixture, parameters_fixture):
    response = rest_client.load(model_fixture, parameters_fixture)
    assert not response.error, response.error_msg
    model = response.html
    request.cls.model = model

    yield  # perform testing

    try:
        rest_client.unload(model)
    except ConnectionError:
        pass


@pytest.fixture(scope="session")
def rest_client(request):
    server_addr = get_server_addr(request.config)
    return RestClient(server_addr)


@pytest.fixture(scope="session")
def ws_client(request):
    server_addr = get_server_addr(request.config)
    return WebsocketClient(server_addr)


@pytest.fixture(autouse=True, scope="class")
def assign_client(request, rest_client, ws_client):
    request.cls.rest_client = rest_client
    request.cls.ws_client = ws_client
