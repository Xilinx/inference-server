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

"""
The setup.py is used for local installation during development. It just installs
the *.py files in src/ when invoked by CMake and not skbuild. It relies on CMake
to copy over the pybind11 library to the site-packages.

To build a distributable PyPi package, invoke skbuild from the top-level
directory instead e.g.:
    python3 -m pip wheel /some/dir --wheel-dir /another/dir --no-deps
"""

import os

import setuptools

version_path = os.getenv("AMDINFER_ROOT")
if version_path:
    version_path += "/VERSION"
    with open(version_path, "r") as f:
        version = f.read()
else:
    version = "0.0.0"

setuptools.setup(
    name="amdinfer",
    version=version,
    license="Apache 2.0",
    packages=setuptools.find_packages("src"),
    install_requires=[
        "numpy",
        "opencv-python-headless",
    ],
    python_requires=">=3.6",
    package_dir={"": "src"},
    package_data={
        "": [
            "*.pyi",
        ]
    },
    zip_safe=False,  # required for mypy
)
