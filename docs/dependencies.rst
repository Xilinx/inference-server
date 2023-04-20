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

Dependencies
============

This document catalogues the tools, libraries and packages that AMD Inference Server depends on and are installed in the Docker images.
The information here is derived from the Dockerfile and source files used in AMD Inference Server.
If there is a conflict, the source takes precedence.

Docker Image
------------

There are two kinds of images: development and deployment.
Dev images are larger in size and contain all the tools and dependencies to compile, lint and test the inference server.
In contrast, the deployment image is much smaller and only includes specific files from a subset of these packages that are needed at runtime.
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
    :superscript:`4`,Used if MIGraphX is added

Base Image
^^^^^^^^^^

The base image for both dev and deployment images is `Ubuntu 20.04 <https://hub.docker.com/_/ubuntu>`__ and it includes a number of packages by default.

Ubuntu Focal Repositories
^^^^^^^^^^^^^^^^^^^^^^^^^

The following packages (and any dependencies) are installed from Ubuntu's repositories using Ubuntu's package manager ``apt``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :ubuntuPackages:`bash-completion`,1:2.10-1ubuntu1,GPL-2+,Scripts used to auto-complete bash commands\ :superscript:`d 0`
    :ubuntuPackages:`clang-format-10`,1:10.0.0-4ubuntu1,Apache 2.0 with LLVM exceptions + others,Executable used to apply formatting\ :superscript:`d 0`
    :ubuntuPackages:`clang-tidy-10`,1:10.0.0-4ubuntu1,Apache 2.0 with LLVM exceptions + others,Executable used for linting\ :superscript:`d 0`
    :ubuntuPackages:`curl`,7.68.0-1ubuntu2.14,Curl,Executable used for transferring data\ :superscript:`a 0`
    :ubuntuPackages:`doxygen`,1.8.17-0ubuntu2,GPL-2 with Qt exception + others,Executable used for building documentation\ :superscript:`d 0`
    :ubuntuPackages:`gcc`,4:9.3.0-1ubuntu2,GPL-2 + others,Executable used for compiling source code\ :superscript:`d 0`
    :ubuntuPackages:`gdb`,9.2-0ubuntu1~20.04.1,LGPL-2.1+ + others,Executable used for debugging\ :superscript:`d 0`
    :ubuntuPackages:`git`,1:2.25.1-1ubuntu3.6,GPL-2 + others,Executable used for source control\ :superscript:`d 0`
    :ubuntuPackages:`gnupg2`,2.2.19-3ubuntu2.2,GPL-3+ + others,Executable used for signing commits\ :superscript:`d 0`
    :ubuntuPackages:`graphviz`,2.42.2-3build2,EPL-1.0 + others,Executable used to draw graphs in documentation\ :superscript:`d 0`
    :ubuntuPackages:`libboost1.71-dev`,1.71.0-6ubuntu6,Boost,Used to build AKS kernels\ :superscript:`a 1`
    :ubuntuPackages:`libboost-filesystem1.71.0`,1.71.0-6ubuntu6,Boost,Dynamically linked by AKS\ :superscript:`a 1`
    :ubuntuPackages:`libboost-serialization1.71.0`,1.71.0-6ubuntu6,Boost,Dynamically linked by AKS\ :superscript:`a 1`
    :ubuntuPackages:`libboost-system1.71.0`,1.71.0-6ubuntu6,Boost,Dynamically linked by AKS\ :superscript:`a 1`
    :ubuntuPackages:`libboost-thread1.71.0`,1.71.0-6ubuntu6,Boost,Dynamically linked by AKS\ :superscript:`a 1`
    :ubuntuPackages:`libbrotli-dev`,1.0.7-6ubuntu0.1,MIT,Dynamically linked by Drogon\ :superscript:`a 0`
    :ubuntuPackages:`libgoogle-glog-dev`,0.4.0-1build1,BSD-3,Dynamically linked by VART\ :superscript:`a 1`
    :ubuntuPackages:`libnuma1`,2.0.12-1,LGPL-2,Dependency of migraphx\ :superscript:`a 4`
    :ubuntuPackages:`libssl-dev`,1.1.1f-1ubuntu2.16,Dual OpenSSL/SSLeay,Dynamically linked by Drogon\ :superscript:`a 0`
    :ubuntuPackages:`locales`,2.31-0ubuntu9.9,GPL-2 + others,Executable used to set locale\ :superscript:`a 0`
    :ubuntuPackages:`make`,4.2.1-1.2,GPL-3+,Executable used to build executables\ :superscript:`d 0`
    :ubuntuPackages:`net-tools`,1.60+git20180626.aebd88e-1ubuntu1,GPL-2+,Executable used to query used ports\ :superscript:`a 1`
    :ubuntuPackages:`openssh-client`,1:8.2p1-4ubuntu0.5,OpenSSH + others,Executable used for remote connections\ :superscript:`d 0`
    :ubuntuPackages:`pkg-config`,0.29.1-0ubuntu4,GPL-2+,Executable used for configuring unilog\ :superscript:`d 0`
    :ubuntuPackages:`python3`,3.8.2-0ubuntu2,PSF License,Executable used for scripting and testing amdinfer-server\ :superscript:`d 0`
    :ubuntuPackages:`python3-dev`,3.8.2-0ubuntu2,PSF License,Used to build Python bindings\ :superscript:`d 0`
    :ubuntuPackages:`sudo`,1.8.31-1ubuntu1.2,ISC license + others,Executable used to grant elevated permissions to the user\ :superscript:`a 0`
    :ubuntuPackages:`symlinks`,1.4-4,Freely distributable,Executable used to convert absolute symlinks to relative ones\ :superscript:`d 0`
    :ubuntuPackages:`tzdata`,2022f-0ubuntu0.20.04.1,Public Domain,Used for setting the timezone\ :superscript:`a 0`
    :ubuntuPackages:`uuid-dev`,2.34-0.1ubuntu9.3,GPL-2+ + others,Statically linked by Drogon\ :superscript:`a 0`
    :ubuntuPackages:`valgrind`,1:3.15.0-1ubuntu9.1,GPL-2+ + others,Executable used for for debugging\ :superscript:`d 0`
    :ubuntuPackages:`vim`,2:8.1.2269-1ubuntu5.9,Vim + others,Executable used for text editing in terminal\ :superscript:`d 0`
    :ubuntuPackages:`wget`,1.20.3-1ubuntu2,GPL-3 with OpenSSL exception,Executable used to retrieve files from the internet\ :superscript:`d 0`
    :ubuntuPackages:`zlib1g-dev`,1:1.2.11.dfsg-2ubuntu1.5,Zlib,Dynamically linked by amdinfer-server\ :superscript:`a 0`

Ubuntu PPAs
^^^^^^^^^^^

The following packages (and any dependencies) are installed from a Personal Package Archive (PPA) using Ubuntu's package manager ``apt``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    `migraphx-dev <http://repo.radeon.com/rocm/apt/5.4.1/pool/main/m/migraphx-dev/migraphx-dev_2.4.0.50401-84~20.04_amd64.deb>`__,2.4.0.50401-84~20.04,MIT,Dynamically linked by amdinfer-server for AMD GPU support\ :superscript:`a 4`
    `miopen-hip-dev <http://repo.radeon.com/rocm/apt/5.4.1/pool/main/m/miopen-hip-dev/miopen-hip-dev_2.19.0.50401-84~20.04_amd64.deb>`__,2.19.0.50401-84~20.04,MIT,Dependency of migraphx\ :superscript:`a 4`
    `rocblas-dev <http://repo.radeon.com/rocm/apt/5.4.1/pool/main/r/rocblas-dev/rocblas-dev_2.46.0.50401-84~20.04_amd64.deb>`__,2.46.0.50401-84~20.04,MIT,Dependency of migraphx\ :superscript:`a 4`
    `rocm-device-libs <http://repo.radeon.com/rocm/apt/5.4.1/pool/main/r/rocm-device-libs/rocm-device-libs_1.0.0.50401-84~20.04_amd64.deb>`__,1.0.0.50401-84~20.04,MIT,Dependency of migraphx\ :superscript:`a 4`

PyPI
^^^^

The following packages (and any dependencies) are installed from the Python Package Index (PyPI) using ``pip``.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :pypiPackages:`black`,latest,MIT,Formatting Python files\ :superscript:`d 0`
    :pypiPackages:`breathe`,latest,BSD-3,Connect Doxygen to Sphinx for documentation\ :superscript:`d 0`
    :pypiPackages:`cmakelang`,latest,GPL-3,CMake linter and formatter\ :superscript:`d 0`
    :pypiPackages:`cpplint`,latest,BSD-3,C++ linter\ :superscript:`d 0`
    :pypiPackages:`exhale`,latest,BSD-3,Documentation generator\ :superscript:`d 0`
    :pypiPackages:`fastcov`,latest,MIT,Reporting test coverage\ :superscript:`d 0`
    :pypiPackages:`numpy`,latest,BSD-3,Scientific computing package for Python\ :superscript:`d 0`
    :pypiPackages:`opencv-python-headless`,latest,MIT,Python bindings for OpenCV\ :superscript:`d 0`
    :pypiPackages:`pip`,latest,MIT,Python package installer\ :superscript:`d 0`
    :pypiPackages:`pre-commit`,latest,MIT,Pre-commit hook framework\ :superscript:`d 0`
    :pypiPackages:`pybind11_mkdoc`,latest,MIT,Used to extract function documentation for Python binding\ :superscript:`d 0`
    :pypiPackages:`pybind11-stubgen`,latest,BSD-3,Used to generate type stubs for Python binding\ :superscript:`d 0`
    :pypiPackages:`pytest`,latest,MIT,Python testing infrastructure\ :superscript:`d 0`
    :pypiPackages:`pytest-benchmark`,latest,BSD-2,Plugin for Pytest to add benchmarking\ :superscript:`d 0`
    :pypiPackages:`pytest-cpp`,latest,MIT,Plugin for Pytest to run C++ tests\ :superscript:`d 0`
    :pypiPackages:`pytest-xprocess`,latest,MIT,Plugin for Pytest to run external processes\ :superscript:`d 0`
    :pypiPackages:`requests`,latest,Apache-2.0,Making REST requests\ :superscript:`d 0`
    :pypiPackages:`rich`,latest,MIT,Printing tables when benchmarking\ :superscript:`d 0`
    :pypiPackages:`setuptools`,latest,MIT,Manage Python packages\ :superscript:`d 0`
    :pypiPackages:`Sphinx`,latest,BSD-2 + others,Building documentation\ :superscript:`d 0`
    :pypiPackages:`sphinx-argparse`,latest,MIT,Sphinx plugin for documenting CLIs\ :superscript:`d 0`
    :pypiPackages:`sphinx-copybutton`,latest,MIT,Adds copy button for code blocks\ :superscript:`d 0`
    :pypiPackages:`sphinxemoji`,latest,BSD-3,Sphinx plugin for enabling emoji\ :superscript:`d 0`
    :pypiPackages:`sphinx-issues`,latest,MIT,Sphinx plugin for links to the project's Github issue tracker\ :superscript:`d 0`
    :pypiPackages:`sphinx-tabs`,latest,MIT,Sphinx plugin to create tabs\ :superscript:`d 0`
    :pypiPackages:`sphinx_tippy`,latest,MIT,Sphinx plugin to create tooltips\ :superscript:`d 0`
    :pypiPackages:`sphinxcontrib-confluencebuilder`,latest,BSD-2,Sphinx plugin to export documentation to Confluence\ :superscript:`d 0`
    :pypiPackages:`sphinxcontrib-jquery`,latest,BSD-0,Sphinx plugin to add JQuery\ :superscript:`d 0`
    :pypiPackages:`sphinxcontrib-openapi`,latest,BSD-2,Sphinx plugin to build OpenAPI docs\ :superscript:`d 0`
    :pypiPackages:`wheel`,latest,MIT,Support wheels for Python packages\ :superscript:`d 0`

Github
^^^^^^

The following packages are installed from Github.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :github:`c-ares/c-ares`,1.14,c-ares license,Dynamically linked by Drogon\ :superscript:`a 0`
    :github:`Kitware/CMake`,3.21.1,BSD-3 + others,Executable used to generate build systems\ :superscript:`d 0`
    :github:`cameron314/concurrentqueue`,1.0.3,Dual BSD-2/Boost + others,Statically linked by amdinfer-server for an efficient multi-producer queue\ :superscript:`a 0`
    :github:`jarro2783/cxxopts`,2.2.1,MIT,Statically linked by amdinfer-server for command-line argument parsing\ :superscript:`a 0`
    :github:`gdraheim/docker-systemctl-replacement`,1.5.4505,EUPL,Executable created by pyinstaller for starting XRM\ :superscript:`a 0`
    :github:`drogonframework/drogon`,1.8.1,MIT,Dynamically linked by amdinfer-server for an HTTP and websocket server\ :superscript:`a 0`
    :github:`SpartanJ/efsw`,latest,MIT,Dynamically linked by amdinfer-server for directory monitoring\ :superscript:`a 0`
    :github:`FFmpeg/FFmpeg`,3.4.8,LGPL-2.1+ + others,Dynamically linked by amdinfer-server for video processing\ :superscript:`a 0`
    :github:`tschaub/gh-pages`,latest,MIT,Executable used to publish documentation to gh-pages branch\ :superscript:`d 0`
    :github:`git-lfs/git-lfs`,2.13.3,MIT + others,Executable used to manage large files in git\ :superscript:`d 0`
    :github:`tianon/gosu`,1.12,Apache 2.0,Executable used to drop down to user when starting container\ :superscript:`a 0`
    :github:`google/googletest`,1.11.0,BSD-3,Statically linked by amdinfer's test executables\ :superscript:`d 0`
    :github:`grpc/grpc`,1.44,Apache 2.0,Statically linked by amdinfer-server for gRPC support\ :superscript:`a 0`
    :github:`include-what-you-use/include-what-you-use`,0.14,LLVM License,Executable used to check C++ header inclusions\ :superscript:`d 0`
    :github:`jemalloc/jemalloc`,5.3.0,BSD-2,Dynamically linked by amdinfer-server for memory allocation implementation\ :superscript:`a 3`
    :github:`json-c/json-c`,0.15,MIT,Dynamically linked by Vitis libraries\ :superscript:`a 1`
    :github:`libb64/libb64`,2.0.0.1,Public Domain Certification,Statically linked by amdinfer-server for base64 codec\ :superscript:`a 0`
    :github:`linux-test-project/lcov`,1.15,GPL-2,Executable used for test coverage measurement\ :superscript:`d 0`
    :github:`opencv/opencv`,3.4.3,Apache 2.0,Dynamically linked by amdinfer-server for image and video processing\ :superscript:`a 0`
    :github:`open-telemetry/opentelemetry-cpp`,1.1.0,Apache 2.0,Dynamically linked by amdinfer-server\ :superscript:`a 0`
    :github:`pybind/pybind11`,2.9.1,BSD-3,Headers used to build Python bindings\ :superscript:`d 0`
    :github:`jupp0r/prometheus-cpp`,0.12.2,MIT,Dynamically linked by amdinfer-server for metrics\ :superscript:`a 0`
    :github:`protocolbuffers/protobuf`,3.19.4,BSD-3,Dynamically linked by amdinfer-server and Vitis libraries\ :superscript:`a 0`
    :github:`fpagliughi/sockpp`,e5c51b5,BSD-3,Dynamically linked by amdinfer-server :superscript:`a 0`
    :github:`gabime/spdlog`,1.8.2,MIT,Statically linked by amdinfer-server for logging\ :superscript:`a 0`
    :github:`Xilinx/Vitis-AI`,3.0,Apache 2.0,VART is dynamically linked by amdinfer-server\ :superscript:`a 1`
    :github:`wg/wrk`,4.1.0,modified Apache 2.0,Executable used for benchmarking amdinfer-server\ :superscript:`d 0`

Others
^^^^^^

The following packages are installed from other online sources.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    `half <https://sourceforge.net/projects/half/>`__,2.2.0,MIT,Used for fp16 datatype

Xilinx
^^^^^^

The following packages are installed from Xilinx.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    :xilinxDownload:`XRM <xrm_202120.1.3.29_18.04-x86_64.deb>`,1.3.29,Apache 2.0,Used for FPGA resource management\ :superscript:`a 1`
    :xilinxDownload:`XRT <xrt_202120.2.12.427_18.04-amd64-xrt.deb>`,2.12.427,Apache 2.0,Used for communicating to the FPGA\ :superscript:`a 1`

AMD
^^^

The following packages are downloaded from AMD.

.. csv-table::
    :header: Name,Version,License,Usage
    :widths: auto

    `PT_v1.12.0_ZenDNN_v4.0_C++_API.zip <https://www.amd.com/en/developer/zendnn.html>`__,4.0,AMD ZenDNN EULA,Used by PT+ZenDNN worker\ :superscript:`a 3`
    `TF_v2.10_ZenDNN_v4.0_C++_API.zip <https://www.amd.com/en/developer/zendnn.html>`__,4.0,AMD ZenDNN EULA,Used by TF+ZenDNN worker\ :superscript:`a 2`


Included
--------

The following files are included in the AMD Inference Server repository under the terms of their original licensing. This information is duplicated in the LICENSE.

.. csv-table::
    :header: Name,Source,Original File,License,Usage
    :widths: auto

    bicycle-384566_640.jpg,`Pixabay <https://pixabay.com/photos/bicycle-bike-biking-sport-cycle-384566/>`__,`bicycle-384566_640.jpg <https://cdn.pixabay.com/photo/2014/07/05/08/18/bicycle-384566_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing\ :superscript:`d 0`
    CodeCoverage.cmake,:github:`bilke/cmake-modules`,`CodeCoverage.cmake <https://github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake>`__,BSD-3,CMake module for test coverage measurement\ :superscript:`d 0`
    crowd.jpg,`Flickr <https://www.flickr.com/photos/mattmangum/2306189268/>`__,`2306189268_88cc86b30f_z.jpg <https://farm3.staticflickr.com/2009/2306189268_88cc86b30f_z.jpg>`__,`CC BY 2.0 <https://creativecommons.org/licenses/by/2.0/legalcode>`_,Used for testing\ :superscript:`d 0`
    ctpl.hpp,:github:`vit-vit/CTPL`,`ctpl.h <https://github.com/vit-vit/CTPL/blob/master/ctpl.h>`__,Apache 2.0,C++ Thread pool library\ :superscript:`a 0`
    dog-3619020_640.jpg,`Pixabay <https://pixabay.com/photos/dog-spitz-smile-ginger-home-pet-3619020/>`__,`dog-3619020_640.jpg <https://cdn.pixabay.com/photo/2018/08/20/14/08/dog-3619020_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing\ :superscript:`d 0`
    nine_9273.jpg,`Keras MNIST dataset <https://keras.io/api/datasets/mnist/>`__,?,`CC BY-SA 3.0 <https://creativecommons.org/licenses/by-sa/3.0/legalcode>`__,Used for testing\ :superscript:`d 0`
    migraphx_bert.py,:github:`ROCmSoftwarePlatform/AMDMIGraphX`,`bert-squad-migraphx.py <https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/blob/develop/examples/nlp/python_bert_squad/bert-squad-migraphx.py>`__,MIT,Python example for YoloV4 model\ :superscript:`d 0`
    migraphx_yolo.py,:github:`ROCmSoftwarePlatform/AMDMIGraphX`,`yolov4_inference.ipynb <https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/blob/develop/examples/vision/python_yolov4/yolov4_inference.ipynb>`__,MIT,Python example for Bert model\ :superscript:`d 0`
    sport-1284275_640.jpg,`Pixabay <https://pixabay.com/photos/sport-skateboard-skateboarding-fun-1284275/>`__,`sport-1284275_640.jpg <https://cdn.pixabay.com/photo/2016/03/27/21/05/sport-1284275_640.jpg>`__,`Pixabay License <https://pixabay.com/service/license/>`_,Used for testing\ :superscript:`d 0`
    yolo_image_processing.py,:github:`ROCmSoftwarePlatform/AMDMIGraphX`,`image_processing.py <https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/blob/develop/examples/vision/python_yolov4/image_processing.py>`__,MIT,Yolo post-processing\ :superscript:`d 0`

Downloaded Files
----------------

The following files can be optionally downloaded by scripts and may be needed by examples and tests.

.. csv-table::
    :header: Name,Source,License,Usage
    :widths: auto
    :escape: ~

    `Physicsworks.ogv <https://upload.wikimedia.org/wikipedia/commons/c/c4/Physicsworks.ogv>`__,`Wikimedia <https://commons.wikimedia.org/wiki/File:Physicsworks.ogv>`__,`CC Attribution 3.0 Unported <https://creativecommons.org/licenses/by/3.0/legalcode>`__,Used for testing\ :superscript:`d 0`
    `girl-1867092_640.jpg <https://cdn.pixabay.com/photo/2016/11/29/03/35/girl-1867092_640.jpg>`__,`Pixabay <https://pixabay.com/photos/girl-model-portrait-smile-smiling-1867092/>`__,`Pixabay License <https://pixabay.com/service/license/>`__,Used for testing\ :superscript:`d 0`
    :xilinxDownload:`vitis_ai_runtime_r1.3.0_image_video.tar.gz`,Xilinx~, Inc.,Apache 2.0,Used for testing\ :superscript:`d 0`
    :xilinxDownload:`densebox_320_320-u200-u250-r1.4.0.tar.gz`,Xilinx~, Inc.,Apache 2.0,Used for testing\ :superscript:`d 1`
    :xilinxDownload:`resnet_v1_50_tf-u200-u250-r1.4.0.tar.gz`,Xilinx~, Inc.,Apache 2.0,Used for testing\ :superscript:`d 1`
    :xilinxDownload:`yolov3_voc-u200-u250-r1.4.0.tar.gz`,Xilinx~, Inc.,Apache 2.0,Used for testing\ :superscript:`d 1`
    :xilinxDownload:`pt_resnet50_imagenet_224_224_8.2G_2.5_1.0_Z3.3.zip`,Xilinx~, Inc.,Apache 2.0,Used for testing\ :superscript:`d 3`
    :xilinxDownload:`tf_resnetv1_50_imagenet_224_224_6.97G_2.5_1.0_Z3.3.zip`,Xilinx~, Inc.,Apache 2.0,Used for testing\ :superscript:`d 2`
    :githubOnnx:`resnet50-v2-7.onnx <vision/classification/resnet/model/resnet50-v2-7.onnx>`,ONNX,Apache 2.0,Used for testing\ :superscript:`d 4`
    `val.txt <https://github.com/mvermeulen/rocm-migraphx/raw/master/datasets/imagenet/val.txt>`__,AMD~, Inc.,?,Used for testing\ :superscript:`d 4`
    :githubOnnx:`yolov4_anchors.txt <vision/object_detection_segmentation/yolov4/dependencies/yolov4_anchors.txt>`,ONNX,Apache 2.0,Used for testing\ :superscript:`d 4`
    :githubOnnx:`yolov4.onnx <vision/object_detection_segmentation/yolov4/model/yolov4.onnx>`,ONNX,Apache 2.0,Used for testing\ :superscript:`d 4`
    :githubOnnx:`coco.names <vision/object_detection_segmentation/yolov4/dependencies/coco.names>`,ONNX,Apache 2.0,Used for testing\ :superscript:`d 4`
    :githubOnnx:`bertsquad-10.onnx <text/machine_comprehension/bert-squad/model/bertsquad-10.onnx>`,ONNX,Apache 2.0,Used for testing\ :superscript:`d 4`
    `run_onnx_squad <https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/raw/develop/examples/nlp/python_bert_squad/run_onnx_squad.py>`__,AMD~, Inc.,Apache 2.0,Used for testing\ :superscript:`d 4`
    `inputs_amd.json <https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/raw/develop/examples/nlp/python_bert_squad/inputs_amd.json>`__,AMD~, Inc.,MIT,Used for testing\ :superscript:`d 4`
