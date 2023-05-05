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

from conan import ConanFile

# in the dockerfile:
# RUN pip install --no-cache-dir conan

# ARG AMDINFER_ROOT
# COPY . $AMDINFER_ROOT

# RUN conan profile detect \
#     && cp $AMDINFER_ROOT/external/conan/ubuntu.cfg $(conan profile path default) \
#     && conan export $AMDINFER_ROOT/external/conan/ffmpeg --version 3.4.8 --user xilinx --channel inference-server \
#     && conan export $AMDINFER_ROOT/external/conan/nodejs --version 14.16.0 --user xilinx --channel inference-server  \
#     && conan export $AMDINFER_ROOT/external/conan/opencv --version 3.4.3 --user xilinx --channel inference-server \
#     && conan export $AMDINFER_ROOT/external/conan/opentelemetry-proto --version 0.19.0 --user xilinx --channel inference-server \
#     && conan export $AMDINFER_ROOT/external/conan/trantor --version 1.5.8 --user xilinx --channel inference-server  \
#     && mkdir -p ~/.conan2/output-folder \
#     && conan install --user xilinx --channel inference-server --build missing --output-folder ~/.conan2/output-folder $AMDINFER_ROOT


class AMDinferRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("b64/2.0.0.1")
        self.requires("concurrentqueue/1.0.3")
        self.requires("cxxopts/2.2.1")
        self.requires("drogon/1.8.2")
        self.requires("grpc/1.44.0@xilinx/inference-server", force=True)
        self.requires("half/2.2.0")
        self.requires("libwebp/1.2.4", override=True)
        self.requires("nodejs/14.16.0@xilinx/inference-server")
        self.requires("opencv/3.4.3@xilinx/inference-server")
        self.requires("opentelemetry-cpp/1.8.1")
        self.requires(
            "opentelemetry-proto/0.19.0@xilinx/inference-server", override=True
        )
        self.requires("openssl/1.1.1s", override=True)
        self.requires("prometheus-cpp/1.1.0")
        self.requires("protobuf/3.19.4", force=True)
        self.requires("pybind11/2.9.1")
        self.requires("spdlog/1.11.0")
        self.requires("trantor/1.5.8@xilinx/inference-server", override=True)
        self.requires("tomlplusplus/3.3.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.22.1]")

        self.test_requires("benchmark/1.7.1")
        self.test_requires("gtest/1.11.0")

    def configure(self):
        self.options["drogon"].with_orm = False
        self.options["prometheus-cpp"].with_pull = False
        self.options["prometheus-cpp"].with_push = False
        self.options["opentelemetry-cpp"].shared = True
        self.options["opentelemetry-cpp"].with_abseil = False
        self.options["opentelemetry-cpp"].with_jaeger = False
        self.options["opentelemetry-cpp"].with_stl = True
        self.options["opentelemetry-cpp"].with_zipkin = False
