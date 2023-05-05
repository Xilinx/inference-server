# Copyright 2019 Conan.io
# Copyright 2023 Advanced Micro Devices, Inc.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os

from conan import ConanFile
from conan.tools.files import copy, get, save
from conan.tools.layout import basic_layout

required_conan_version = ">=1.52.0"


class OpenTelemetryProtoConan(ConanFile):
    name = "opentelemetry-proto"
    license = "Apache-2.0"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/open-telemetry/opentelemetry-proto"
    description = "Protobuf definitions for the OpenTelemetry protocol (OTLP)"
    topics = ("opentelemetry", "telemetry", "otlp")
    package_type = "unknown"
    settings = "os", "arch", "compiler", "build_type"
    no_copy_source = True

    def layout(self):
        basic_layout(self, src_folder="src")

    def package_id(self):
        self.info.clear()

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def package(self):
        copy(
            self,
            pattern="LICENSE",
            dst=os.path.join(self.package_folder, "licenses"),
            src=self.source_folder,
        )
        copy(
            self,
            pattern="*.proto",
            dst=os.path.join(self.package_folder, "res"),
            src=self.source_folder,
        )
        # satisfy KB-H014 (header_only recipes require headers)
        save(self, os.path.join(self.package_folder, "include", "dummy_header.h"), "\n")

    def package_info(self):
        self.conf_info.define(
            "user.opentelemetry-proto:proto_root",
            os.path.join(self.package_folder, "res"),
        )
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []
        self.cpp_info.resdirs = []

        self.user_info.proto_root = os.path.join(self.package_folder, "res")
