..
    Copyright 2021 Xilinx, Inc.
    Copyright 2022 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Command-Line Interface
======================

The :file:`amdinfer` script provides an easy way to launch containers and build the server during development.
The documentation for the options available in this Python script can be seen here or on the terminal using the ``--help`` flag.

.. argparse::
   :module: amdinfer_cli
   :func: get_parser
   :prog: amdinfer
