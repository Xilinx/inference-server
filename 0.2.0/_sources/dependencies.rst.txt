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

This document catalogues the tools, libraries and packages that AMD Inference Server depends on and are installed in the AMD Inference Server Docker containers.
The information here is derived from the Dockerfile and source files used in AMD Inference Server.
If there is a conflict, the source takes precedence.

Docker Image
------------

There are two kinds of images: development and production.
Dev images are larger in size and contain all the tools and dependencies to compile, lint and test the inference server.
In contrast, the production image is much smaller and only includes specific files from a subset of these packages that are needed at runtime.
In these two images, multiple flavors of Docker images may be created based on which optional features are enabled and how the server is compiled.
In the tables below, note the following symbols to indicate where a package (or a subset of the package) or its dependencies are used:

.. csv-table::
    :header: Symbol,Meaning
    :widths: 10, 90
    :width: 22em

    :superscript:`a`,Used by all images
    :superscript:`d`,Used by dev images only
    :superscript:`0`,Used always (subject to compilation options)
    :superscript:`1`,Used if Vitis AI is added
    :superscript:`2`,Used if TF+ZenDNN is added
    :superscript:`3`,Used if PT+ZenDNN is added

Base Image
^^^^^^^^^^

The base image for both dev and production images is `Ubuntu 18.04 <https://hub.docker.com/_/ubuntu>`__ and it includes a number of packages by default.

Ubuntu Bionic Repositories
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following packages (and any dependencies) are installed from Ubuntu's repositories using Ubuntu's package manager ``apt``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :ubuntupackages:`bash-completion`,1:2.8-1ubuntu1,GPL-2+,Scripts used to auto-complete bash commands\ :superscript:`d 0`
    :ubuntupackages:`clang-format-10`,1:10.0.0-4ubuntu1~18.04.2,Apache 2.0 with LLVM exceptions + others,Executable used to apply formatting\ :superscript:`d 0`
    :ubuntupackages:`clang-tidy-10`,1:10.0.0-4ubuntu1~18.04.2,Apache 2.0 with LLVM exceptions + others,Executable used for linting\ :superscript:`d 0`
    :ubuntupackages:`curl`,7.58.0-2ubuntu3.14,Curl,Executable used for transferring data\ :superscript:`a 0`
    :ubuntupackages:`doxygen`,1.8.13-10,GPL-2 with Qt exception + others,Executable used for building documentation\ :superscript:`d 0`
    :ubuntupackages:`gcc`,4:7.4.0-1ubuntu2.3,GPL-2 + others,Executable used for compiling source code\ :superscript:`d 0`
    :ubuntupackages:`gdb`,8.1-0ubuntu3,LGPL-2.1+ + others,Executable used for debugging\ :superscript:`d 0`
    :ubuntupackages:`git`,1:2.17.1-1ubuntu0.8,GPL-2 + others,Executable used for source control\ :superscript:`d 0`
    :ubuntupackages:`graphviz`,2.40.1-2,EPL-1.0 + others,Executable used to draw graphs in documentation\ :superscript:`d 0`
    :ubuntupackages:`libboost1.65-dev`,1.65.1+dfsg-0ubuntu5,Boost,Used to build AKS kernels\ :superscript:`a 1`
    :ubuntupackages:`libboost-filesystem1.65.1`,1.65.1+dfsg-0ubuntu5,Boost,Dynamically linked by AKS\ :superscript:`a 1`
    :ubuntupackages:`libboost-serialization1.65.1`,1.65.1+dfsg-0ubuntu5,Boost,Dynamically linked by AKS\ :superscript:`a 1`
    :ubuntupackages:`libboost-system1.65.1`,1.65.1+dfsg-0ubuntu5,Boost,Dynamically linked by AKS\ :superscript:`a 1`
    :ubuntupackages:`libboost-thread1.65.1`,1.65.1+dfsg-0ubuntu5,Boost,Dynamically linked by AKS\ :superscript:`a 1`
    :ubuntupackages:`libbrotli-dev`,1.0.3-1ubuntu1.3,MIT,Dynamically linked by Drogon\ :superscript:`a 0`
    :ubuntupackages:`libc-ares-dev`,1.14.0-1ubuntu0.1,MIT + others,Dynamically linked by Drogon\ :superscript:`a 0`
    :ubuntupackages:`libgoogle-glog-dev`,0.3.5-1,BSD-3,Dynamically linked by VART\ :superscript:`a 1`
    :ubuntupackages:`libjson-c-dev`,0.12.1-1.3ubuntu0.3,MIT,Dynamically linked by rt-engine\ :superscript:`a 1`
    :ubuntupackages:`libjsoncpp-dev`,1.7.4-3,MIT + others,Dynamically linked by proteus-server and Drogon\ :superscript:`a 0`
    :ubuntupackages:`libssl-dev`,1.1.1-1ubuntu2.1~18.04.13,Dual OpenSSL/SSLeay,Dynamically linked by Drogon\ :superscript:`a 0`
    :ubuntupackages:`locales`,2.27-3ubuntu1.2,GPL-2 + others,Executable used to set locale\ :superscript:`a 0`
    :ubuntupackages:`make`,4.1-9.1ubuntu1,GPL-3+,Executable used to build executables\ :superscript:`d 0`
    :ubuntupackages:`net-tools`,1.60+git20161116.90da8a0-1ubuntu1,GPL-2+,Executable used to query used ports\ :superscript:`a 1`
    :ubuntupackages:`openssh-client`,1:7.6p1-4ubuntu0.5,OpenSSH + others,Executable used for remote connections\ :superscript:`d 0`
    :ubuntupackages:`pkg-config`,0.29.1-0ubuntu2,GPL-2+,Executable used for configuring unilog\ :superscript:`d 0`
    :ubuntupackages:`python3`,3.6.5-3,PSF License,Executable used for scripting and testing proteus-server\ :superscript:`d 0`
    :ubuntupackages:`python3-dev`,3.6.7-1~18.04,PSF License,Used to build Python bindings\ :superscript:`d 0`
    :ubuntupackages:`sudo`,1.8.21p2-3ubuntu1.4,ISC license + others,Executable used to grant elevated permissions to the user\ :superscript:`a 0`
    :ubuntupackages:`symlinks`,1.4-3build1,Freely distributable,Executable used to convert absolute symlinks to relative ones\ :superscript:`d 0`
    :ubuntupackages:`tzdata`,2021a-0ubuntu0.18.04,Public Domain,Used for setting the timezone\ :superscript:`a 0`
    :ubuntupackages:`uuid-dev`,2.31.1-0.4ubuntu3.7,GPL-2+ + others,Dynamically linked by Drogon\ :superscript:`a 0`
    :ubuntupackages:`valgrind`,1:3.13.0-2ubuntu2,GPL-2+ + others,Executable used for for debugging\ :superscript:`d 0`
    :ubuntupackages:`vim`,2:8.0.1453-1ubuntu1.4,Vim + others,Executable used for text editing in terminal\ :superscript:`d 0`
    :ubuntupackages:`wget`,1.19.4-1ubuntu2.2,GPL-3 with OpenSSL exception,Executable used to retrieve files from the internet\ :superscript:`d 0`
    :ubuntupackages:`zlib1g-dev`,1:1.2.11.dfsg-0ubuntu2,Zlib,Dynamically linked by proteus-server\ :superscript:`a 0`

Ubuntu PPAs
^^^^^^^^^^^

The following packages (and any dependencies) are installed from a Personal Package Archive (PPA) using Ubuntu's package manager ``apt``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    `gcc-9 <https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test>`__,9.4.0-1ubuntu1~18.04,GPL-3 + others,Executable used for compiling source code\ :superscript:`d 0`
    `g++-9 <https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test>`__,9.4.0-1ubuntu1~18.04,GPL-3 + others,Executable used for compiling source code\ :superscript:`d 0`

PyPI
^^^^

The following packages (and any dependencies) are installed from the Python Package Index (PyPI) using ``pip``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :pypipackages:`black`,latest,MIT,Formatting Python files\ :superscript:`d 0`
    :pypipackages:`breathe`,latest,BSD-3,Connect Doxygen to Sphinx for documentation\ :superscript:`d 0`
    :pypipackages:`cmakelang`,latest,GPL-3,CMake linter and formatter\ :superscript:`d 0`
    :pypipackages:`cpplint`,latest,BSD-3,C++ linter\ :superscript:`d 0`
    :pypipackages:`fastcov`,latest,MIT,Reporting test coverage\ :superscript:`d 0`
    :pypipackages:`numpy`,latest,BSD-3,Scientific computing package for Python\ :superscript:`d 0`
    :pypipackages:`opencv-python-headless`,latest,MIT,Python bindings for OpenCV\ :superscript:`d 0`
    :pypipackages:`pip`,latest,MIT,Python package installer\ :superscript:`d 0`
    :pypipackages:`pre-commit`,latest,MIT,Pre-commit hook framework\ :superscript:`d 0`
    :pypipackages:`pybind11_mkdoc`,latest,MIT,Used to extract function documentation for Python binding\ :superscript:`d 0`
    :pypipackages:`pybind11-stubgen`,latest,BSD-3,Used to generate type stubs for Python binding\ :superscript:`d 0`
    :pypipackages:`pytest`,latest,MIT,Python testing infrastructure\ :superscript:`d 0`
    :pypipackages:`pytest-benchmark`,latest,BSD-2,Plugin for Pytest to add benchmarking\ :superscript:`d 0`
    :pypipackages:`pytest-cpp`,latest,MIT,Plugin for Pytest to run C++ tests\ :superscript:`d 0`
    :pypipackages:`pytest-xprocess`,latest,MIT,Plugin for Pytest to run external processes\ :superscript:`d 0`
    :pypipackages:`requests`,latest,Apache-2.0,Making REST requests\ :superscript:`d 0`
    :pypipackages:`rich`,latest,MIT,Printing tables when benchmarking\ :superscript:`d 0`
    :pypipackages:`setuptools`,latest,MIT,Manage Python packages\ :superscript:`d 0`
    :pypipackages:`sphinx`,latest,BSD-2 + others,Building documentation\ :superscript:`d 0`
    :pypipackages:`sphinx-argparse`,latest,MIT,Sphinx plugin for documenting CLIs\ :superscript:`d 0`
    :pypipackages:`sphinx_copybutton`,latest,MIT,Adds copy button for code blocks\ :superscript:`d 0`
    :pypipackages:`sphinx-issues`,latest,MIT,Sphinx plugin for links to the project's Github issue tracker\ :superscript:`d 0`
    :pypipackages:`sphinxcontrib-confluencebuilder`,latest,BSD-2,Sphinx plugin to export documentation to Confluence\ :superscript:`d 0`
    :pypipackages:`wheel`,latest,MIT,Support wheels for Python packages\ :superscript:`d 0`

Github
^^^^^^

The following packages are installed from Github.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :github:`Kitware/CMake`,3.21.1,BSD-3 + others,Executable used to generate build systems\ :superscript:`d 0`
    :github:`cameron314/concurrentqueue`,1.0.3,Dual BSD-2/Boost + others,Statically linked by proteus-server for an efficient multi-producer queue\ :superscript:`a 0`
    :github:`jarro2783/cxxopts`,2.2.1,MIT,Statically linked by proteus-server for command-line argument parsing\ :superscript:`a 0`
    :github:`gdraheim/docker-systemctl-replacement`,1.5.4505,EUPL,Executable created by pyinstaller for starting XRM\ :superscript:`a 0`
    :github:`drogonframework/drogon`,1.7.5,MIT,Dynamically linked by proteus-server for an HTTP and websocket server\ :superscript:`a 0`
    :github:`SpartanJ/efsw`,latest,MIT,Dynamically linked by proteus-server for directory monitoring\ :superscript:`a 0`
    :github:`FFmpeg/FFmpeg`,3.4.8,LGPL-2.1+ + others,Dynamically linked by proteus-server for video processing\ :superscript:`a 0`
    :github:`tschaub/gh-pages`,latest,MIT,Executable used to publish documentation to gh-pages branch\ :superscript:`d 0`
    :github:`git-lfs/git-lfs`,2.13.3,MIT + others,Executable used to manage large files in git\ :superscript:`d 0`
    :github:`tianon/gosu`,1.12,Apache 2.0,Executable used to drop down to user when starting container\ :superscript:`a 0`
    :github:`google/googletest`,1.11.0,BSD-3,Statically linked by proteus's test executables\ :superscript:`d 0`
    :github:`grpc/grpc`,1.44,Apache 2.0,Statically linked by proteus-server for gRPC support\ :superscript:`a 0`
    :github:`include-what-you-use/include-what-you-use`,0.14,LLVM License,Executable used to check C++ header inclusions\ :superscript:`d 0`
    :github:`jemalloc/jemalloc`,5.3.0,BSD-2,Dynamically linked by proteus-server for memory allocation implementation\ :superscript:`a 3`
    :github:`json-c/json-c`,0.15,MIT,Dynamically linked by Vitis libraries\ :superscript:`a 1`
    :github:`libb64/libb64`,2.0.0.1,Public Domain Certification,Statically linked by proteus-server for base64 codec\ :superscript:`a 0`
    :github:`linux-test-project/lcov`,1.15,GPL-2,Executable used for test coverage measurement\ :superscript:`d 0`
    :github:`nodejs/node`,14.16.0,MIT + others,Executable used for web GUI development\ :superscript:`d 0`
    :github:`opencv/opencv`,3.4.3,Apache 2.0,Dynamically linked by proteus-server for image and video processing\ :superscript:`a 0`
    :github:`open-telemetry/opentelemetry-cpp`,1.1.0,Apache 2.0,Dynamically linked by proteus-server\ :superscript:`a 0`
    :github:`pybind/pybind11`,2.9.1,BSD-3,Headers used to build Python bindings\ :superscript:`d 0`
    :github:`jupp0r/prometheus-cpp`,0.12.2,MIT,Dynamically linked by proteus-server for metrics\ :superscript:`a 0`
    :github:`protocolbuffers/protobuf`,3.19.4,BSD-3,Dynamically linked by proteus-server and Vitis libraries\ :superscript:`a 0`
    :github:`gabime/spdlog`,1.8.2,MIT,Statically linked by proteus-server for logging\ :superscript:`a 0`
    :github:`Xilinx/Vitis-AI`,latest,Apache 2.0,VART is dynamically linked by proteus-server\ :superscript:`a 1`
    :github:`wg/wrk`,4.1.0,modified Apache 2.0,Executable used for benchmarking proteus-server\ :superscript:`d 0`

Xilinx
^^^^^^

The following packages are installed from Xilinx.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :xilinxdownload:`XRM <xrm_202120.1.3.29_18.04-x86_64.deb>`,1.3.29,Apache 2.0,Used for FPGA resource management\ :superscript:`a 1`
    :xilinxdownload:`XRT <xrt_202120.2.12.427_18.04-amd64-xrt.deb>`,2.12.427,Apache 2.0,Used for communicating to the FPGA\ :superscript:`a 1`

AMD
^^^

The following packages are downloaded from AMD.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    `TF_v2.9_ZenDNN_v3.3_C++_API.zip <https://developer.amd.com/zendnn/>`__,3.3,AMD ZenDNN EULA,Used by TF+ZenDNN worker\ :superscript:`a 2`
    `PT_v1.11.0_ZenDNN_v3.3_C++_API.zip <https://developer.amd.com/zendnn/>`__,3.3,AMD ZenDNN EULA,Used by PT+ZenDNN worker\ :superscript:`a 3`

Included
--------

The following files are included in the AMD Inference Server repository under the terms of their original licensing. This information is duplicated in the LICENSE.

.. csv-table::
    :header: Name,Source,Original File,License,Usage
    :widths: auto

    bicycle-384566_640.jpg,`Pixabay <https://pixabay.com/photos/bicycle-bike-biking-sport-cycle-384566/>`__,`bicycle-384566_640.jpg <https://cdn.pixabay.com/photo/2014/07/05/08/18/bicycle-384566_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing\ :superscript:`d 0`
    CodeCoverage.cmake,:github:`bilke/cmake-modules`,`CodeCoverage.cmake <https://www.github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake>`__,BSD-3,CMake module for test coverage measurement\ :superscript:`d 0`
    ctpl.h,:github:`vit-vit/CTPL`,`ctpl.h <https://www.github.com/vit-vit/CTPL/blob/master/ctpl.h>`__,Apache 2.0,C++ Thread pool library\ :superscript:`a 0`
    dog-3619020_640.jpg,`Pixabay <https://pixabay.com/photos/dog-spitz-smile-ginger-home-pet-3619020/>`__,`dog-3619020_640.jpg <https://cdn.pixabay.com/photo/2018/08/20/14/08/dog-3619020_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing\ :superscript:`d 0`
    nine_9273.jpg,`Keras MNIST dataset <https://keras.io/api/datasets/mnist/>`__,?,`CC BY-SA 3.0 <https://creativecommons.org/licenses/by-sa/3.0/legalcode>`__,Used for testing\ :superscript:`d 0`
    proteusConfig.cmake,:github:`alexreinking/SharedStaticStarter`,`SomeLibConfig.cmake <https://www.github.com/alexreinking/SharedStaticStarter/blob/master/packaging/SomeLibConfig.cmake>`__,MIT,CMake module for installing libraries\ :superscript:`d 0`
    Queue.js,`Kate Rose Morley <https://code.iamkate.com/javascript/queues/>`__,`Queue.src.js <https://code.iamkate.com/javascript/queues/Queue.src.js>`__,`CC0 1.0 Universal <https://creativecommons.org/publicdomain/zero/1.0/legalcode>`__,JavaScript class for a queue\ :superscript:`d 0`
    sport-1284275_640.jpg,`Pixabay <https://pixabay.com/photos/sport-skateboard-skateboarding-fun-1284275/>`__,`sport-1284275_640.jpg <https://cdn.pixabay.com/photo/2016/03/27/21/05/sport-1284275_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing\ :superscript:`d 0`

Downloaded Files
----------------

The following files can be optionally downloaded by scripts and may be needed by examples and tests.

.. csv-table::
    :header: Name,Source,License,Usage
    :widths: auto

    :xilinxdownload:`densebox_320_320-u200-u250-r1.4.0.tar.gz <densebox_320_320-u200-u250-r1.4.0.tar.gz>`,Xilinx Inc.,Apache 2.0,Used for testing\ :superscript:`d 0`
    `girl-1867092_640.jpg <https://cdn.pixabay.com/photo/2016/11/29/03/35/girl-1867092_640.jpg>`__,`Pixabay <https://pixabay.com/photos/girl-model-portrait-smile-smiling-1867092/>`__,`Pixabay License <https://pixabay.com/service/license/>`__,Used for testing\ :superscript:`d 0`
    `Physicsworks.ogv <https://upload.wikimedia.org/wikipedia/commons/c/c4/Physicsworks.ogv>`__,`Wikimedia <https://commons.wikimedia.org/wiki/File:Physicsworks.ogv>`__,`CC Attribution 3.0 Unported <https://creativecommons.org/licenses/by/3.0/legalcode>`__,Used for testing\ :superscript:`d 0`
    :xilinxdownload:`pt_resnet50_imagenet_224_224_8.2G_2.5_1.0_Z3.3.zip <pt_resnet50_imagenet_224_224_8.2G_2.5_1.0_Z3.3.zip>`,Xilinx Inc.,Apache 2.0,Used for testing\ :superscript:`d 3`
    :xilinxdownload:`resnet_v1_50_tf-u200-u250-r1.4.0.tar.gz <resnet_v1_50_tf-u200-u250-r1.4.0.tar.gz>`,Xilinx Inc.,Apache 2.0,Used for testing\ :superscript:`d 0`
    :xilinxdownload:`tf_resnetv1_50_imagenet_224_224_6.97G_2.5_1.0_Z3.3.zip <tf_resnetv1_50_imagenet_224_224_6.97G_2.5_1.0_Z3.3.zip>`,Xilinx Inc.,Apache 2.0,Used for testing\ :superscript:`d 2`
    :xilinxdownload:`vitis_ai_runtime_r1.3.0_image_video.tar.gz <vitis_ai_runtime_r1.3.0_image_video.tar.gz>`,Xilinx Inc.,Apache 2.0,Used for testing\ :superscript:`d 0`
    :xilinxdownload:`yolov3_adas_pruned_0_9-u200-u250-r1.4.0.tar.gz <yolov3_adas_pruned_0_9-u200-u250-r1.4.0.tar.gz>`,Xilinx Inc.,Apache 2.0,Used for testing\ :superscript:`d 0`
    :xilinxdownload:`yolov3_voc-u200-u250-r1.4.0.tar.gz <yolov3_voc-u200-u250-r1.4.0.tar.gz>`,Xilinx Inc.,Apache 2.0,Used for testing\ :superscript:`d 0`
