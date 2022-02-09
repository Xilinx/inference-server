..
    Copyright 2021 Xilinx Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

.. _dependencies:

Dependencies
============

This document catalogues the tools, libraries and packages that Xilinx Inference Server depends on and are installed in the Xilinx Inference Server Docker containers.
The information here is derived from the Dockerfile and source files used in Xilinx Inference Server.
If there is a conflict, the source takes precedence.

Docker Image
------------

Base Image
^^^^^^^^^^

The base image for Xilinx Inference Server containers is `Ubuntu 18.04 <https://hub.docker.com/_/ubuntu>`__ and it includes a number of packages by default.

Ubuntu Bionic Repositories
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following packages (and any dependencies) are installed in the Xilinx Inference Server dev container from Ubuntu's repositories using Ubuntu's package manager ``apt``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :ubuntupackages:`bash_completion`,1:2.8-1ubuntu1,GPL-2+,Scripts used to auto-complete bash commands
    :ubuntupackages:`clang-format-10`,1:10.0.0-4ubuntu1~18.04.2,Apache 2.0 with LLVM exceptions + others,Executable used to apply formatting
    :ubuntupackages:`clang-tidy-10`,1:10.0.0-4ubuntu1~18.04.2,Apache 2.0 with LLVM exceptions + others,Executable used for linting
    :ubuntupackages:`curl`,7.58.0-2ubuntu3.14,Curl,Executable used for transferring data
    :ubuntupackages:`doxygen`,1.8.13-10,GPL-2 with Qt exception + others,Executable used for building documentation
    :ubuntupackages:`gcc`,4:7.4.0-1ubuntu2.3,GPL-2 + others,Executable used for compiling source code
    :ubuntupackages:`gdb`,8.1-0ubuntu3,LGPL-2.1+ + others,Executable used for debugging
    :ubuntupackages:`git`,1:2.17.1-1ubuntu0.8,GPL-2 + others,Executable used for source control
    :ubuntupackages:`graphviz`,2.40.1-2,EPL-1.0 + others,Executable used to draw graphs in documentation
    :ubuntupackages:`libbrotli-dev`,1.0.3-1ubuntu1.3,MIT,Dynamically linked by Drogon
    :ubuntupackages:`libc-ares-dev`,1.14.0-1ubuntu0.1,MIT + others,Dynamically linked by Drogon
    :ubuntupackages:`libjson-c-dev`,0.12.1-1.3ubuntu0.3,MIT,Dynamically linked by rt-engine
    :ubuntupackages:`libjsoncpp-dev`,1.7.4-3,MIT + others,Dynamically linked by proteus-server and Drogon
    :ubuntupackages:`libssl-dev`,1.1.1-1ubuntu2.1~18.04.13,Dual OpenSSL/SSLeay,Dynamically linked by Drogon
    :ubuntupackages:`locales`,2.27-3ubuntu1.2,GPL-2 + others,Executable used to set locale
    :ubuntupackages:`make`,4.1-9.1ubuntu1,GPL-3+,Executable used to build executables
    :ubuntupackages:`net-tools`,1.60+git20161116.90da8a0-1ubuntu1,GPL-2+,Executable used to query used ports
    :ubuntupackages:`openssh-client`,1:7.6p1-4ubuntu0.5,OpenSSH + others,Executable used for remote connections
    :ubuntupackages:`pkg-config`,0.29.1-0ubuntu2,GPL-2+,Executable used for configuring unilog
    :ubuntupackages:`python3`,3.6.5-3,PSF License,Executable used for scripting and testing proteus-server
    :ubuntupackages:`sudo`,1.8.21p2-3ubuntu1.4,ISC license + others,Executable used to grant elevated permissions to the user
    :ubuntupackages:`symlinks`,1.4-3build1,Freely distributable,Executable used to convert absolute symlinks to relative ones
    :ubuntupackages:`tzdata`,2021a-0ubuntu0.18.04,Public Domain,Used for setting the timezone
    :ubuntupackages:`uuid-dev`,2.31.1-0.4ubuntu3.7,GPL-2+ + others,Dynamically linked by Drogon
    :ubuntupackages:`valgrind`,1:3.13.0-2ubuntu2,GPL-2+ + others,Executable used for for debugging
    :ubuntupackages:`vim`,2:8.0.1453-1ubuntu1.4,Vim + others,Executable used for text editing in terminal
    :ubuntupackages:`wget`,1.19.4-1ubuntu2.2,GPL-3 with OpenSSL exception,Executable used to retrieve files from the internet
    :ubuntupackages:`zlib1g-dev`,1:1.2.11.dfsg-0ubuntu2,Zlib,Dynamically linked by proteus-server

Ubuntu PPAs
^^^^^^^^^^^

The following packages (and any dependencies) are installed in the Xilinx Inference Server dev container from a Personal Package Archive (PPA) using Ubuntu's package manager ``apt``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    `gcc-9 <https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test>`__,9.4.0-1ubuntu1~18.04,GPL-3 + others,Executable used for compiling source code
    `g++-9 <https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test>`__,9.4.0-1ubuntu1~18.04,GPL-3 + others,Executable used for compiling source code

PyPI
^^^^

The following packages (and any dependencies) are installed in the Xilinx Inference Server dev container from the Python Package Index (PyPI) using ``pip``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :pypipackages:`aiohttp`,latest,Apache 2.0,Async HTTP client
    :pypipackages:`black`,latest,MIT,Formatting Python files
    :pypipackages:`breathe`,latest,BSD-3,Connect Doxygen to Sphinx for documentation
    :pypipackages:`cpplint`,latest,BSD-3,C++ linter
    :pypipackages:`fastcov`,latest,MIT,Reporting test coverage
    :pypipackages:`numpy`,latest,BSD-3,Scientific computing package for Python
    :pypipackages:`opencv-python-headless`,latest,MIT,Python bindings for OpenCV
    :pypipackages:`pip`,latest,MIT,Python package installer
    :pypipackages:`pytest`,latest,MIT,Python testing infrastructure
    :pypipackages:`pytest-benchmark`,latest,BSD-2,Plugin for Pytest to add benchmarking
    :pypipackages:`requests`,latest,Apache-2.0,Making REST requests
    :pypipackages:`rich`,latest,MIT,Printing tables when benchmarking
    :pypipackages:`setuptools`,latest,MIT,Manage Python packages
    :pypipackages:`sphinx`,latest,BSD-2 + others,Building documentation
    :pypipackages:`sphinx-argparse`,latest,MIT,Sphinx plugin for documenting CLIs
    :pypipackages:`sphinx_copybutton`,latest,MIT,Adds copy button for code blocks
    :pypipackages:`sphinxcontrib-confluencebuilder`,latest,BSD-2,Sphinx plugin to export documentation to Confluence
    :pypipackages:`websocket-client`,latest,Apache-2.0,Using websockets in Python
    :pypipackages:`wheel`,latest,MIT,Support wheels for Python packages

Github
^^^^^^

The following packages are installed in the Xilinx Inference Server dev container from the Github.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :github:`Kitware/CMake`,3.21.1,BSD-3 + others,Executable used to generate build systems
    :github:`cameron314/concurrentqueue`,1.0.3,Dual BSD-2/Boost + others,Statically linked by proteus-server for an efficient multi-producer queue
    :github:`jarro2783/cxxopts`,2.2.1,MIT,Statically linked by proteus-server for command-line argument parsing
    :github:`gdraheim/docker-systemctl-replacement`,1.5.4505,EUPL,Executable created by pyinstaller for starting XRM
    :github:`drogonframework/drogon`,1.3.0,MIT,Dynamically linked by proteus-server for an HTTP and websocket server
    :github:`tschaub/gh-pages`,latest,MIT,Executable used to publish documentation to gh-pages branch
    :github:`git-lfs/git-lfs`,2.13.3,MIT + others,Executable used to manage large files in git
    :github:`tianon/gosu`,1.12,Apache 2.0,Executable used to drop down to user when starting container
    :github:`google/googletest`,1.11.0,BSD-3,Statically linked by proteus's test executables
    :github:`include-what-you-use/include-what-you-use`,0.14,LLVM License,Executable used to check C++ header inclusions
    :github:`json-c/json-c`,0.15,MIT,Dynamically linked by Vitis libraries
    :github:`libb64/libb64`,2.0.0.1,Public Domain Certification,Statically linked by proteus-server for base64 codec
    :github:`linux-test-project/lcov`,1.15,GPL-2,Executable used for test coverage measurement
    :github:`nodejs/node`,14.16.0,MIT + others,Executable used for web GUI development
    :github:`opencv/opencv`,3.4.4,Apache 2.0,Dynamically linked by proteus-server for image and video processing
    :github:`open-telemetry/opentelemetry-cpp`,1.1.0,Apache 2.0,Dynamically linked by proteus-server
    :github:`jupp0r/prometheus-cpp`,0.12.2,MIT,Dynamically linked by proteus-server for metrics
    :github:`protocolbuffers/protobuf`,3.4.0,BSD-3,Dynamically linked by proteus-server and Vitis libraries
    :github:`gabime/spdlog`,1.8.2,MIT,Statically linked by proteus-server for logging
    :github:`wg/wrk`,4.1.0,modified Apache 2.0,Executable used for benchmarking proteus-server

Xilinx
^^^^^^

The following packages are installed in the Xilinx Inference Server dev container from Xilinx using Ubuntu's package manager ``apt``.

.. csv-table::
    :header: Name,Version,Link,License
    :widths: auto

    aks,1.4.0-r73,:xilinxdownload:`Debian package <aks_1.4.0-r73_amd64.deb>`,Apache 2.0
    rt-engine,1.4.0-r178,:xilinxdownload:`Debian package <librt-engine_1.4.0-r178_amd64.deb>`,Apache 2.0
    target-factory,1.4.0-r77,:xilinxdownload:`Debian package <libtarget-factory_1.4.0-r77_amd64.deb>`,Apache 2.0
    unilog,1.4.0-r75,:xilinxdownload:`Debian package <libunilog_1.4.0-r75_amd64.deb>`,Apache 2.0
    vart,1.4.0-r117,:xilinxdownload:`Debian package <libvart_1.4.0-r117_amd64.deb>`,Apache 2.0
    vitis-ai-library,1.4.0-r105,:xilinxdownload:`Debian package <libvitis_ai_library_1.4.0-r105_amd64.deb>`,Apache 2.0
    xir,1.4.0-r80,:xilinxdownload:`Debian package <libxir_1.4.0-r80_amd64.deb>`,Apache 2.0
    xrm,1.3.29,:xilinxdownload:`Debian package <xrm_202120.1.3.29_18.04-x86_64.deb>`,Apache 2.0
    xrt,2.12.427,:xilinxdownload:`Debian package <xrt_202120.2.12.427_18.04-amd64-xrt.deb>`,Apache 2.0

Included
--------

The following files are included in the Xilinx Inference Server repository under the terms of their original licensing. This information is duplicated in the LICENSE.

.. csv-table::
    :header: Name,Source,Original File,License,Usage
    :widths: auto

    bicycle-384566_640.jpg,`Pixabay <https://pixabay.com/photos/bicycle-bike-biking-sport-cycle-384566/>`__,`bicycle-384566_640.jpg <https://cdn.pixabay.com/photo/2014/07/05/08/18/bicycle-384566_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing
    CodeCoverage.cmake,:github:`bilke/cmake-modules`,`CodeCoverage.cmake <https://www.github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake>`__,BSD-3,Cmake module for test coverage measurement
    ctpl.h,:github:`vit-vit/CTPL`,`ctpl.h <https://www.github.com/vit-vit/CTPL/blob/master/ctpl.h>`__,Apache 2.0,C++ Thread pool library
    dog-3619020_640.jpg,`Pixabay <https://pixabay.com/photos/dog-spitz-smile-ginger-home-pet-3619020/>`__,`dog-3619020_640.jpg <https://cdn.pixabay.com/photo/2018/08/20/14/08/dog-3619020_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing
    proteusConfig.cmake,:github:`alexreinking/SharedStaticStarter`,`SomeLibConfig.cmake <https://www.github.com/alexreinking/SharedStaticStarter/blob/master/packaging/SomeLibConfig.cmake>`__,MIT,Cmake module for installing libraries
    Queue.js,`Kate Rose Morley <https://code.iamkate.com/javascript/queues/>`__,`Queue.src.js <https://code.iamkate.com/javascript/queues/Queue.src.js>`__,`CC0 1.0 Universal <https://creativecommons.org/publicdomain/zero/1.0/legalcode>`__,JavaScript class for a queue
    sport-1284275_640.jpg,`Pixabay <https://pixabay.com/photos/sport-skateboard-skateboarding-fun-1284275/>`__,`sport-1284275_640.jpg <https://cdn.pixabay.com/photo/2016/03/27/21/05/sport-1284275_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing

Downloaded Files
----------------

The following files can be optionally downloaded by scripts and may be needed by examples and tests.

.. csv-table::
    :header: Name,Source,License
    :widths: auto

    :xilinxdownload:`densebox_320_320-u200-u250-r1.4.0.tar.gz <densebox_320_320-u200-u250-r1.4.0.tar.gz>`,Xilinx Inc.,Apache 2.0
    `girl-1867092_640.jpg <https://cdn.pixabay.com/photo/2016/11/29/03/35/girl-1867092_640.jpg>`__,`Pixabay <https://pixabay.com/photos/girl-model-portrait-smile-smiling-1867092/>`__,`Pixabay License <https://pixabay.com/service/license/>`__
    `Physicsworks.ogv <https://upload.wikimedia.org/wikipedia/commons/c/c4/Physicsworks.ogv>`__,`Wikimedia <https://commons.wikimedia.org/wiki/File:Physicsworks.ogv>`__,`CC Attribution 3.0 Unported <https://creativecommons.org/licenses/by/3.0/legalcode>`__
    :xilinxdownload:`resnet_v1_50_tf-u200-u250-r1.4.0.tar.gz <resnet_v1_50_tf-u200-u250-r1.4.0.tar.gz>`,Xilinx Inc.,Apache 2.0
    :xilinxdownload:`vitis_ai_runtime_r1.3.0_image_video.tar.gz <vitis_ai_runtime_r1.3.0_image_video.tar.gz>`,Xilinx Inc.,Apache 2.0
    :xilinxdownload:`yolov3_adas_pruned_0_9-u200-u250-r1.4.0.tar.gz <yolov3_adas_pruned_0_9-u200-u250-r1.4.0.tar.gz>`,Xilinx Inc.,Apache 2.0
    :xilinxdownload:`yolov3_voc-u200-u250-r1.4.0.tar.gz <yolov3_voc-u200-u250-r1.4.0.tar.gz>`,Xilinx Inc.,Apache 2.0
