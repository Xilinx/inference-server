#!/usr/bin/python3
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


import json
import os
import socket
import sys
from pathlib import Path


class XRMClient:
    def __init__(self, host, port):
        self.sock = socket.socket()
        try:
            self.sock.connect((host, port))
        except socket.error as msg:
            msg.strerror = "Failed to reach the XRM daemon."
            self.sock.close()
            raise msg

    def send(self, data):
        try:
            self.sock.sendall(data)
        except socket.error as msg:
            msg.strerror = "Failed to send data to the XRM daemon."
            self.sock.close()
            raise msg

    def receive(self):
        total_data = bytearray()
        total_len = 0
        cur_len = 0
        while True:
            cur_data = self.sock.recv(131072)
            if cur_data:
                if total_len == 0:
                    total_len = int.from_bytes(cur_data[0:4], byteorder="little")
                    cur_len = len(cur_data) - 4
                    total_data = bytearray(cur_data[4:])
                else:
                    cur_len += len(cur_data)
                    total_data += bytearray(cur_data)
                if cur_len >= total_len:
                    break
            else:
                break
        return bytes(total_data)

    def close(self):
        self.sock.close()


def send_request(request: dict):
    data = json.dumps(request).encode("utf-8")

    try:
        cli = XRMClient("localhost", 9763)
        cli.send(data)
    except socket.error as msg:
        print(msg.strerror)
        sys.exit(1)
    except Exception as ex:
        print("Failed: " + type(ex).__name__)
        sys.exit(1)

    parsed_output = json.loads(cli.receive(), object_pairs_hook=dict)
    cli.close()
    return parsed_output


def get_device_data():
    request = {"request": {"name": "list", "requestId": 1}}

    output = send_request(request)
    response_data = output["response"]["data"]

    fpgas = []
    try:
        num_fpgas = int(response_data["deviceNumber"])
        for i in range(num_fpgas):
            fpgas.append(response_data["device_" + str(i)])
    except:
        print("Failed to parse response from XRM")
        sys.exit(1)

    return fpgas


def get_devices():
    devices = get_device_data()

    device_str = ""
    for device in devices:
        dsa = device["dsaName    "]
        device_str += f"{dsa},"

    # if it's non-empty, trim the trailing comma
    if device_str:
        return device_str[:-1]
    return device_str


def get_kernels():
    devices = get_device_data()

    fpgas = {}
    for device in devices:
        if "cuNumber   " not in device:
            continue
        num_cus = int(device["cuNumber   "])
        for j in range(num_cus):
            kernel = device[f"cu_{str(j)}"]["kernelName   "]
            if kernel not in fpgas:
                fpgas[kernel] = 1
            else:
                fpgas[kernel] = fpgas[kernel] + 1

    fpgas_str = ""
    for key, value in fpgas.items():
        fpgas_str += key + ":" + str(value) + ","

    # if it's non-empty, trim the trailing comma
    if fpgas_str:
        return fpgas_str[:-1]
    return fpgas_str


def load_fpga(device_id, xclbin):
    request = {
        "request": {
            "name": "load",
            "requestId": 1,
            "parameters": [{"device": device_id, "xclbin": xclbin}],
        }
    }

    output = send_request(request)

    return output["response"]["status"] != "failed"


def unload_fpga(device_id):
    request = {
        "request": {
            "name": "unload",
            "requestId": 1,
            "parameters": {"device": [device_id]},
        }
    }

    output = send_request(request)

    return output["response"]["status"] == "ok"


def load_fpgas():
    devices = get_device_data()

    try:
        xclbins_root = os.environ["XLNX_VART_FIRMWARE"]
    except KeyError:
        xclbins_root = "/opt/xilinx/overlaybins"

    xclbins = list(Path(xclbins_root).rglob("*.xclbin"))

    for index, device in enumerate(devices):
        if "cuNumber   " not in device:
            for xclbin in xclbins:
                # keep trying xclbins until the first one succeeds
                if load_fpga(index, str(xclbin)):
                    break


def print_help():
    print("Usage: fpga-util command")
    print("  get-devices: print the shell of each FPGA in order")
    print("  get-kernels: print the number of kernels of each type")
    print("  load <index> <xclbin>: load a xclbin to a particular FPGA")
    print("  load-all: attempt to load valid xclbins to all FPGAs")
    print("  unload <index>: unload the xclbin of a particular FPGA")
    sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print_help()

    arg = sys.argv[1]
    if arg == "get-kernels":
        print(get_kernels())
    elif arg == "get-devices":
        print(get_devices())
    elif arg == "load":
        if len(sys.argv) != 4:
            print_help()
        if not load_fpga(int(sys.argv[2]), str(sys.argv[3])):
            sys.exit(2)
    elif arg == "load-all":
        load_fpgas()
    elif arg == "unload":
        if len(sys.argv) != 3:
            print_help()
        if not unload_fpga(int(sys.argv[2])):
            sys.exit(2)
    else:
        print_help()
