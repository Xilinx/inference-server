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

import os
from pathlib import Path

import setuptools
import skbuild

version = Path("VERSION").read_text("utf-8")
readme = Path("README.rst").read_text("utf-8")
package = Path("src/amdinfer/bindings/python/src")

# add the following to the README for the description page on PyPI
pypi_description = """\

IMPORTANT NOTICE CONCERNING OPEN-SOURCE SOFTWARE

Materials in this release may be licensed by Xilinx or third parties and may
be subject to the GNU General Public License, the GNU Lesser General License,
or other licenses.

Licenses and source files may be downloaded from:

* `libamdinfer <https://github.com/Xilinx/inference-server>`__
* `libbrotlicommon <https://rhel.pkgs.org/7/epel-x86_64/libbrotli-1.0.9-10.el7.x86_64.rpm.html>`__
* `libbrotlidec <https://rhel.pkgs.org/7/epel-x86_64/libbrotli-1.0.9-10.el7.x86_64.rpm.html>`__
* `libbrotlienc <https://rhel.pkgs.org/7/epel-x86_64/libbrotli-1.0.9-10.el7.x86_64.rpm.html>`__
* `libcares <https://github.com/c-ares/c-ares/archive/refs/tags/cares-1_14_0.tar.gz>`__
* `libcom_err <https://centos.pkgs.org/7/centos-x86_64/libcom_err-1.42.9-19.el7.x86_64.rpm.html>`__
* `libcrypto <https://centos.pkgs.org/7/centos-x86_64/openssl-devel-1.0.2k-19.el7.x86_64.rpm.html>`__
* `libdrogon <https://github.com/drogonframework/drogon/tree/v1.8.1>`__
* `libefsw <https://github.com/SpartanJ/efsw/tree/6b51944994b5c77dbd7edce66846e378a3bf4d8e>`__
* `libgssapi_krb5 <https://centos.pkgs.org/7/centos-x86_64/krb5-libs-1.15.1-50.el7.x86_64.rpm.html>`__
* `libjsoncpp <https://github.com/open-source-parsers/jsoncpp/tree/1.7.4>`__
* `libk5crypto <https://centos.pkgs.org/7/centos-x86_64/krb5-libs-1.15.1-50.el7.x86_64.rpm.html>`__
* `libkeyutils <https://centos.pkgs.org/7/centos-x86_64/keyutils-1.5.8-3.el7.x86_64.rpm.html>`__
* `libkrb5 <https://centos.pkgs.org/7/centos-x86_64/krb5-devel-1.15.1-50.el7.x86_64.rpm.html>`__
* `libkrb5support <https://centos.pkgs.org/7/centos-x86_64/krb5-devel-1.15.1-50.el7.x86_64.rpm.html>`__
* `libopencv_core <https://github.com/opencv/opencv/tree/3.4.3>`__
* `libopencv_imgcodecs <https://github.com/opencv/opencv/tree/3.4.3>`__
* `libopencv_imgproc <https://github.com/opencv/opencv/tree/3.4.3>`__
* `libossp-uuid <https://centos.pkgs.org/7/centos-x86_64/uuid-devel-1.6.2-26.el7.x86_64.rpm.html>`__
* `libpcre <https://centos.pkgs.org/7/centos-x86_64/pcre-8.32-17.el7.x86_64.rpm.html>`__
* `libprometheus-cpp-core <https://github.com/jupp0r/prometheus-cpp/archive/refs/tags/v0.12.2.tar.gz>`__
* `libprotobuf <https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protobuf-cpp-3.19.4.tar.gz>`__
* `libselinux <https://centos.pkgs.org/7/centos-x86_64/libselinux-2.5-15.el7.x86_64.rpm.html>`__
* `libssl <https://centos.pkgs.org/7/centos-x86_64/openssl-devel-1.0.2k-19.el7.x86_64.rpm.html>`__
* `libtrantor <https://github.com/drogonframework/drogon/tree/v1.8.1>`__

Note:  You are solely responsible for checking the header files and other
accompanying source files (i) provided within, in support of, or that otherwise
accompanies these materials or (ii) created from the use of third party
software and tools (and associated libraries and utilities) that are supplied
with these materials, because such header and/or source files may contain
or describe various copyright notices and license terms and conditions
governing such files, which vary from case to case based on your usage
and are beyond the control of Xilinx. You are solely responsible for
complying with the terms and conditions imposed by third parties as applicable
to your software applications created from the use of third
party software and tools (and associated libraries and utilities) that are
supplied with the materials.
"""

long_description = readme + pypi_description

skbuild.setup(
    name="amdinfer",
    version=version,
    description="Python client library for the AMD Inference Server: unified inference across AMD CPUs, GPUs, and FPGAs",
    long_description=long_description,
    long_description_content_type="text/x-rst",
    license="Apache 2.0",
    author="Varun Sharma",
    author_email="varun.sharma@amd.com",
    url="https://github.com/Xilinx/inference-server",
    project_urls={
        "Bug Tracker": "https://github.com/Xilinx/inference-server/issues",
        "Changelog": "https://xilinx.github.io/inference-server/main/changelog.html",
        "Documentation": "https://xilinx.github.io/inference-server/main/index.html",
        "Source Code": "https://github.com/Xilinx/inference-server",
    },
    packages=setuptools.find_packages(str(package)),
    install_requires=[
        "numpy",
        "opencv-python-headless",
    ],
    python_requires=">=3.6",
    classifiers=[
        "Development Status :: 4 - Beta",
        # "Environment :: Console",
        "Intended Audience :: Science/Research",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: Apache Software License",
        "Natural Language :: English",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: C++",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: Implementation :: CPython",
        "Topic :: Scientific/Engineering",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Scientific/Engineering :: Image Processing",
        "Topic :: Scientific/Engineering :: Image Recognition",
        # "Typing :: Typed",
    ],
    package_dir={"": str(package)},
    include_package_data=True,
    cmake_install_dir=str(package / "amdinfer"),
    package_data={
        "": [
            "*.pyi",
        ]
    },
    zip_safe=False,  # required for mypy
)
