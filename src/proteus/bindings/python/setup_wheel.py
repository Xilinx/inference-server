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

import multiprocessing as mp
import os
import subprocess
import sys

import setuptools
from setuptools.command.build_ext import build_ext

root_path = os.getenv("PROTEUS_ROOT")
assert root_path is not None


class CMakeExtension(setuptools.Extension):
    def __init__(self, name, sourcedir=root_path):
        setuptools.Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))

        # required for auto-detection & inclusion of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE=Release",
        ]
        cmake_args += ["-DCMAKE_BUILD_WITH_INSTALL_RPATH=TRUE"]
        cmake_args += ["-DCMAKE_INSTALL_RPATH={}".format("$ORIGIN")]
        cmake_args += ["-DPROTEUS_BUILD_TESTING=OFF"]
        # cmake_args += ["-DPROTEUS_ENABLE_VITIS=OFF"]
        # cmake_args += ["-DPROTEUS_ENABLE_MIGRAPHX=OFF"]
        # cmake_args += ["-DPROTEUS_ENABLE_TFZENDNN=OFF"]
        # cmake_args += ["-DPROTEUS_ENABLE_PTZENDNN=OFF"]
        # cmake_args += ["-DPROTEUS_ENABLE_AKS=OFF"]

        build_temp = os.path.join(self.build_temp, ext.name)
        if not os.path.exists(build_temp):
            os.makedirs(build_temp)

        os.environ.putenv("CMAKE_BUILD_PARALLEL_LEVEL", str(mp.cpu_count()))
        subprocess.check_call(["cmake", ext.sourcedir] + cmake_args, cwd=build_temp)
        subprocess.check_call(["cmake", "--build", "."], cwd=build_temp)


version_path = root_path + "/VERSION"
with open(version_path, "r") as f:
    version = f.read()

setuptools.setup(
    name="proteus",
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
            "*.so*",
        ]
    },
    zip_safe=False,  # required for mypy
    ext_modules=[CMakeExtension("proteus")],
    cmdclass={"build_ext": CMakeBuild},
)
