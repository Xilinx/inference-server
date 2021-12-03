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

"""
This module provides the Server class.

Classes:
    Server - provides methods to start/stop the Proteus server
"""

import os
import subprocess
from typing import Optional
import shutil

from .exceptions import InvalidArgument


class Server:
    """
    The Server class provides methods to control the Proteus server.
    """

    def __init__(self, executable: Optional[str] = None, http_port: int = 8998):
        """
        Construct a server object

        Args:
            http_port (int): HTTP port to use for the server
        """
        self.pid: int = 0
        self.http_port = http_port

        if executable is None:
            root = os.getenv("PROTEUS_ROOT")
            if root is None:
                if shutil.which("proteus-server") is None:
                    raise InvalidArgument(
                        "Path to proteus-server cannot be derived. Specify the path explicitly, add it to the PATH or set PROTEUS_ROOT in the environment"
                    )
                else:
                    # use the proteus-server that exists on the PATH
                    self.executable = "proteus-server"
            try:
                with open(root + "/build/config.txt", "r") as f:
                    build = f.read().replace("\n", "").split(" ")[0]
            except FileNotFoundError:
                print("No config.txt found in build/. Using default value of 'Debug'")
                build = "Debug"
            self.executable = f"{root}/build/{build}/src/proteus/proteus-server"
        else:
            self.executable = executable

    def start(self, quiet=False):
        """
        Start the proteus server

        Args:
            quiet (bool, optional): Suppress all output if True. Defaults to False.
        """

        proteus_command = [self.executable, "--http_port", str(self.http_port)]
        if quiet:
            p = subprocess.Popen(
                proteus_command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )
        else:
            p = subprocess.Popen(proteus_command)
        self.pid = p.pid

    def stop(self, kill=False):
        """
        Stop the proteus server

        Args:
            kill (bool, optional): Use signal 9 to kill. Defaults to False.
        """
        signal = "-9" if kill else "-2"

        if self.pid:
            subprocess.call(["kill", signal, str(self.pid)])
            self.pid = 0
