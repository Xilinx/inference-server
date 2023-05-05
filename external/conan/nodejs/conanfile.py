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
import re

from conan import ConanFile, conan_version
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import copy, get
from conan.tools.scm import Version
from six import StringIO

required_conan_version = ">=1.59.0"


class NodejsConan(ConanFile):
    name = "nodejs"
    description = "nodejs binaries for use in recipes"
    topics = ("node", "javascript", "runtime")
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://nodejs.org"
    license = "MIT"
    settings = "os", "arch", "compiler"
    no_copy_source = True
    short_paths = True

    @property
    def _source_subfolder(self):
        return os.path.join(self.source_folder, "source_subfolder")

    @property
    def _nodejs_arch(self):
        if str(self.settings.os) == "Linux":
            if str(self.settings.arch).startswith("armv7"):
                return "armv7"
            if str(self.settings.arch).startswith("armv8") and "32" not in str(
                self.settings.arch
            ):
                return "armv8"
        return str(self.settings.arch)

    @property
    def _glibc_version(self):
        cmd = ["ldd", "--version"] if conan_version.major == "1" else ["ldd --version"]
        buff = StringIO()
        self.run(cmd, buff)
        return str(re.search(r"GLIBC (\d{1,3}.\d{1,3})", buff.getvalue()).group(1))

    def validate(self):
        if (
            not self.version in self.conan_data["sources"]
            or not str(self.settings.os) in self.conan_data["sources"][self.version]
            or not self._nodejs_arch
            in self.conan_data["sources"][self.version][str(self.settings.os)]
        ):
            raise ConanInvalidConfiguration(
                "Binaries for this combination of architecture/version/os not available"
            )

        if Version(self.version) >= "18.0.0":
            if str(self.settings.os) == "Linux":
                if Version(self._glibc_version) < "2.27":
                    raise ConanInvalidConfiguration(
                        "Binaries for this combination of architecture/version/os not available"
                    )

    def build(self):
        get(
            self,
            **self.conan_data["sources"][self.version][str(self.settings.os)][
                self._nodejs_arch
            ],
            destination=self._source_subfolder,
            strip_root=True
        )

    def package(self):
        copy(
            self,
            pattern="LICENSE",
            dst=os.path.join(self.package_folder, "licenses"),
            src=self._source_subfolder,
        )
        copy(
            self,
            pattern="*",
            dst=os.path.join(self.package_folder, "bin"),
            src=os.path.join(self._source_subfolder, "bin"),
        )
        copy(
            self,
            pattern="*",
            dst=os.path.join(self.package_folder, "lib"),
            src=os.path.join(self._source_subfolder, "lib"),
        )
        copy(
            self,
            pattern="node.exe",
            dst=os.path.join(self.package_folder, "bin"),
            src=self._source_subfolder,
        )
        copy(
            self,
            pattern="npm",
            dst=os.path.join(self.package_folder, "bin"),
            src=self._source_subfolder,
        )
        copy(
            self,
            pattern="npx",
            dst=os.path.join(self.package_folder, "bin"),
            src=self._source_subfolder,
        )

    def package_info(self):
        self.cpp_info.includedirs = []
        bin_dir = os.path.join(self.package_folder, "bin")
        self.output.info("Appending PATH environment variable: {}".format(bin_dir))
        self.env_info.PATH.append(bin_dir)
