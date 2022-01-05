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

ARG NIGHTLY=stable
ARG PROTEUS_ROOT=/workspace/proteus
ARG BASE_IMAGE=ubuntu:18.04
ARG COPY_DIR=/root/deps

FROM ${BASE_IMAGE} AS proteus_base

ARG UNAME=proteus-user
ARG GNAME=proteus
ARG UID=1000
ARG GID=1000
ARG PROTEUS_ROOT

LABEL project="proteus"

ENV TZ=America/Los_Angeles
ENV LANG=en_US.UTF-8

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        locales \
        # used to detect if ports are in use for XRM
        net-tools \
        sudo \
        tzdata \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    # set up timezone
    && ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone \
    && dpkg-reconfigure --frontend noninteractive tzdata \
    # set up locale
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8

# add a user
RUN groupadd -g $GID -o $GNAME \
    && useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME \
    && passwd -d $UNAME \
    && usermod -aG sudo $UNAME \
    && echo 'ALL ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers \
    && usermod -aG $GNAME root

ENV PROTEUS_ROOT=$PROTEUS_ROOT

FROM proteus_base AS dev_base

# install a newer compiler for all development images
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        gcc \
        # add the add-apt-repository command
        software-properties-common \
        # used to get packages
        wget \
    # install gcc-9 for a newer compiler
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        gcc-9 \
        g++-9 \
    # link gcc-9 and g++-9 to gcc and g++
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 \
        --slave /usr/bin/g++ g++ /usr/bin/g++-9 \
        --slave /usr/bin/gcov gcov /usr/bin/gcov-9 \
    # clean up
    && apt-get -y purge --auto-remove software-properties-common \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

FROM dev_base AS builder
ARG COPY_DIR
WORKDIR /tmp

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        ca-certificates \
        checkinstall \
        git \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# make a directory to hold everything to be copied later
RUN mkdir ${COPY_DIR}

# install gosu 1.12 for dropping down to the user in the entrypoint
RUN wget -O gosu --progress=dot:mega "https://github.com/tianon/gosu/releases/download/1.12/gosu-amd64" \
    && chmod 755 gosu \
    && mkdir -p ${COPY_DIR}/usr/local/bin/ && cp gosu ${COPY_DIR}/usr/local/bin/ \
    && rm -rf /tmp/*

# install git-lfs 2.13.3 for managing large files
RUN wget --progress=dot:mega https://github.com/git-lfs/git-lfs/releases/download/v2.13.3/git-lfs-linux-amd64-v2.13.3.tar.gz \
    && mkdir git-lfs \
    && tar -xzf git-lfs-linux-amd64-v2.13.3.tar.gz -C git-lfs \
    && mkdir -p ${COPY_DIR}/usr/local/bin/ && cp git-lfs/git-lfs ${COPY_DIR}/usr/local/bin/ \
    && rm -rf /tmp/*

# install lcov 1.15 for test coverage measurement
RUN wget --progress=dot:mega https://github.com/linux-test-project/lcov/releases/download/v1.15/lcov-1.15.tar.gz \
    && tar -xzf lcov-1.15.tar.gz \
    && cd lcov-1.15 \
    && checkinstall -y --pkgname lcov --pkgversion 1.15 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L lcov | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install Cmake 3.21.1
RUN wget --progress=dot:mega https://github.com/Kitware/CMake/releases/download/v3.21.1/cmake-3.21.1-linux-x86_64.tar.gz \
    && tar --strip-components=1 -xzf cmake-3.21.1-linux-x86_64.tar.gz  -C /usr/local \
    && mkdir -p {COPY_DIR}/usr/local \
    && tar --strip-components=1 -xzf cmake-3.21.1-linux-x86_64.tar.gz  -C ${COPY_DIR}/usr/local \
    && rm -rf /tmp/*

# install NodeJS 14.16.0 for web gui development
RUN wget --progress=dot:mega https://nodejs.org/dist/v14.16.0/node-v14.16.0-linux-x64.tar.xz \
    && tar --strip-components=1 -xf node-v14.16.0-linux-x64.tar.xz  -C /usr/local \
    && mkdir -p ${COPY_DIR}/usr/local \
    && tar --strip-components=1 -xf node-v14.16.0-linux-x64.tar.xz  -C ${COPY_DIR}/usr/local \
    && rm -rf /tmp/*

# install cxxopts 2.2.1 for argument parsing
RUN wget --progress=dot:mega https://github.com/jarro2783/cxxopts/archive/refs/tags/v2.2.1.tar.gz \
    && tar -xzf v2.2.1.tar.gz \
    && cd cxxopts-2.2.1 \
    && mkdir -p ${COPY_DIR}/usr/local/include/cxxopts \
    && cp include/cxxopts.hpp ${COPY_DIR}/usr/local/include/cxxopts \
    && rm -rf /tmp/*

# install concurrentqueue 1.0.3 for a lock-free multi-producer and consumer queue
RUN wget --progress=dot:mega https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.3.tar.gz \
    && tar -xzf v1.0.3.tar.gz \
    && cd concurrentqueue-1.0.3 \
    && cmake . \
    && checkinstall -y --pkgname concurrentqueue --pkgversion 1.0.3 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L concurrentqueue | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install drogon 1.3.0 for a http server
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        libbrotli-dev \
        libjsoncpp-dev \
        libc-ares-dev \
        libssl-dev \
        uuid-dev \
        zlib1g-dev \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    # symlink libjsoncpp to json to maintain include compatibility with Drogon
    && cp -rfs /usr/include/jsoncpp/json/ /usr/include/ \
    # install drogon
    && cd /tmp && git clone -b v1.3.0 https://github.com/an-tao/drogon \
    && cd drogon \
    && git submodule update --init --recursive \
    && mkdir -p build && cd build \
    && cmake .. \
        -DBUILD_EXAMPLES=OFF \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_ORM=OFF \
        -DBUILD_DROGON_SHARED=ON \
        -DBUILD_CTL=OFF \
    && make -j \
    && checkinstall -y --pkgname drogon --pkgversion 1.3.0 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L drogon | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install libb64 2.0.0.1. The default version in apt adds linebreaks when encoding
RUN wget --progress=dot:mega https://github.com/libb64/libb64/archive/refs/tags/v2.0.0.1.tar.gz \
    && tar -xzf v2.0.0.1.tar.gz \
    && cd libb64-2.0.0.1 \
    && make all_src \
    && mkdir -p ${COPY_DIR}/usr/local/lib && cp src/libb64.a ${COPY_DIR}/usr/local/lib \
    && mkdir -p ${COPY_DIR}/usr/local/include && cp -r include/b64 ${COPY_DIR}/usr/local/include \
    && rm -rf /tmp/*

# install GTest 1.11.0 for C++ testing
RUN wget --progress=dot:mega https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz \
    && tar -xzf release-1.11.0.tar.gz \
    && cd googletest-release-1.11.0 \
    && mkdir -p build && cd build \
    && cmake .. \
    && checkinstall -y --pkgname gtest --pkgversion 1.11.0 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L gtest | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install FFmpeg 3.4.8 for opencv
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        nasm \
    && wget --progress=dot:mega https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n3.4.8.tar.gz \
    && tar -xzf n3.4.8.tar.gz \
    && cd FFmpeg-n3.4.8 \
    && ./configure --disable-static --enable-shared \
    && make -j \
    && checkinstall -y --pkgname ffmpeg --pkgversion 3.4.8 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L ffmpeg | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install opencv 3.4.3 for image and video processing
# pkg-config is needed to find ffmpeg
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        pkg-config \
    && wget --progress=dot:mega https://github.com/opencv/opencv/archive/3.4.3.tar.gz \
    && tar -xzf 3.4.3.tar.gz \
    && cd opencv-3.4.3 \
    && mkdir -p build \
    && cd build \
    && cmake -DBUILD_SHARED_LIBS=ON -DBUILD_TEST=OFF -DBUILD_PERF_TESTS=OFF -DWITH_FFMPEG=ON .. \
    && make -j \
    && checkinstall -y --pkgname opencv --pkgversion 3.4.3 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L opencv | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install protobuf 3.4.0 - dependency on pre-built target-factory, VART and XIR
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        autoconf \
        automake \
        curl \
        libtool \
        unzip \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && VERSION=3.4.0 \
    && wget --progress=dot:mega https://github.com/protocolbuffers/protobuf/releases/download/v${VERSION}/protobuf-cpp-${VERSION}.tar.gz \
    && tar -xzf protobuf-cpp-${VERSION}.tar.gz \
    && cd protobuf-${VERSION} \
    && ./autogen.sh \
    && ./configure \
    && make -j \
    && checkinstall -y --pkgname protobuf --pkgversion ${VERSION} --pkgrelease 1 make install \
    && dpkg -L protobuf | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -fr /tmp/*

# install spdlog 1.8.2
RUN wget --progress=dot:mega https://github.com/gabime/spdlog/archive/refs/tags/v1.8.2.tar.gz \
    && tar -xzf v1.8.2.tar.gz \
    && cd spdlog-1.8.2 \
    && mkdir -p build && cd build \
    && cmake .. \
    && make -j \
    && checkinstall -y --pkgname spdlog --pkgversion 1.8.2 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L spdlog | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install prometheus-cpp 0.12.2 for metrics
RUN wget --progress=dot:mega https://github.com/jupp0r/prometheus-cpp/archive/refs/tags/v0.12.2.tar.gz \
    && tar -xzf v0.12.2.tar.gz \
    && cd prometheus-cpp-0.12.2 \
    && mkdir -p build && cd build \
    && cmake .. \
        -DENABLE_PUSH=OFF \
        -DENABLE_PULL=OFF \
        -DENABLE_TESTING=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DUSE_THIRDPARTY_LIBRARIES=OFF \
    && make -j \
    && checkinstall -y --pkgname prometheus-cpp --pkgversion 0.12.2 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L prometheus-cpp | xargs -i bash -c "if [ -f {} ]; then cp --parents -P -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# get Nlohmann JSON for building opentelemetry
RUN wget --progress=dot:mega https://github.com/nlohmann/json/archive/refs/tags/v3.7.3.tar.gz \
    && tar -xzf v3.7.3.tar.gz \
    && cd json-3.7.3 \
    && mkdir -p build && cd build \
    && cmake .. \
        -DBUILD_TESTING=OFF \
    && make -j \
    && checkinstall -y --pkgname nlohmann-json --pkgversion 3.7.3 --pkgrelease 1 make install \
    && rm -rf /tmp/*

# install Apache Thrift 0.12.0 for running the jaeger exporter in opentelemetry
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        bison \
        flex \
        libboost-all-dev \
    && VERSION=0.15.0 \
    && cd /tmp && wget --progress=dot:mega https://github.com/apache/thrift/archive/refs/tags/v${VERSION}.tar.gz \
    && tar -xzf v${VERSION}.tar.gz \
    && cd thrift-${VERSION} \
    && mkdir -p build && cd build \
    && cmake .. \
        -DBUILD_TESTING=OFF \
        -DBUILD_TUTORIALS=OFF \
        -DBUILD_COMPILER=ON \
        -DBUILD_CPP=ON \
        -DBUILD_C_GLIB=OFF \
        -DBUILD_JAVA=OFF \
        -DBUILD_PYTHON=OFF \
        -DBUILD_JAVASCRIPT=OFF \
        -DBUILD_NODEJS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_SHARED_LIBS=OFF \
    && make -j \
    && checkinstall -y --pkgname thrift --pkgversion ${VERSION} --pkgrelease 1 make install \
    # && cd /tmp \
    # && dpkg -L thrift | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install opentelemetry 1.1.0 for tracing
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        libcurl4-openssl-dev \
    && VERSION=1.1.0 \
    && wget https://github.com/open-telemetry/opentelemetry-cpp/archive/refs/tags/v${VERSION}.tar.gz \
    && tar -xzf v${VERSION}.tar.gz \
    && cd opentelemetry-cpp-${VERSION} \
    && mkdir build && cd build \
    && cmake .. \
        -DWITH_JAEGER=ON \
        -DBUILD_TESTING=OFF \
        -DWITH_EXAMPLES=OFF \
        -DWITH_PROMETHEUS=ON \
        -DWITH_STL=ON \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_SHARED_LIBS=ON \
    && make -j \
    && checkinstall -y --pkgname opentelemetry --pkgversion ${VERSION} --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L opentelemetry | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -rf /tmp/*

# install wrk for http benchmarking
RUN wget --progress=dot:mega https://github.com/wg/wrk/archive/refs/tags/4.1.0.tar.gz \
    && tar -xzf 4.1.0.tar.gz \
    && cd wrk-4.1.0 \
    && make -j \
    && mkdir -p ${COPY_DIR}/usr/local/bin && cp wrk ${COPY_DIR}/usr/local/bin \
    && rm -rf /tmp/*

# install include-what-you-use 0.14
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        libclang-10-dev \
        clang-10 \
        llvm-10-dev \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && wget --progress=dot:mega https://github.com/include-what-you-use/include-what-you-use/archive/refs/tags/0.14.tar.gz \
    && tar -xzf 0.14.tar.gz \
    && cd include-what-you-use-0.14 \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_PREFIX_PATH=/usr/lib/llvm-10 .. \
    && make -j \
    && checkinstall -y --pkgname iwyu --pkgversion 0.14.0 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L iwyu | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -fr /tmp/*

# install json-c 0.15 for Vitis AI 2.0 libraries (used by VART)
RUN wget --progress=dot:mega https://github.com/json-c/json-c/archive/refs/tags/json-c-0.15-20200726.tar.gz \
    && tar -xzf json-c-0.15-20200726.tar.gz \
    && cd json-c-json-c-0.15-20200726 \
    && mkdir build \
    && cd build \
    && cmake .. \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_STATIC_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
    && make -j \
    && checkinstall -y --pkgname json-c --pkgversion 0.15.0 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L json-c | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && rm -fr /tmp/*

# Delete /usr/local/man which is a symlink and cannot be copied later by BuildKit.
# Note: this works without BuildKit: https://github.com/docker/buildx/issues/150
RUN cp -r ${COPY_DIR}/usr/local/man/ ${COPY_DIR}/usr/local/share/man/ \
    && rm -rf ${COPY_DIR}/usr/local/man/

# install doxygen 1.9.2
# RUN cd /tmp && wget --progress=dot:mega https://github.com/doxygen/doxygen/archive/refs/tags/Release_1_9_2.tar.gz \
#     && tar -xzf Release_1_9_2.tar.gz \
#     && cd doxygen-Release_1_9_2/ \
#     && mkdir -p build \
#     && cd build \
#     && cmake ..
#     && make -j \
#     && checkinstall -y --pkgname doxygen --pkgversion 1.9.2 --pkgrelease 1 make install \
#     && cd /tmp \
#     && dpkg -L doxygen | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
#     && rm -fr /tmp/*

FROM dev_base AS proteus_dev

SHELL ["/bin/bash", "-c"]

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        # used for auto-completing bash commands
        bash-completion \
        curl \
        git \
        make \
        # used for git
        openssh-client \
        python3 \
        # install documentation dependencies
        doxygen \
        graphviz \
        # install debugging tools
        gdb \
        valgrind \
        vim \
        # used to turn absolute symlinks into relative ones
        symlinks \
        # used for code formatting and style
        clang-format-10 \
        clang-tidy-10 \
        # used for json objects in proteus since it's also used in Drogon
        libjsoncpp-dev \
        # used for z compression in proteus
        zlib1g-dev \
        # used by drogon. It needs the -dev versions to pass Cmake
        libbrotli-dev \
        libc-ares-dev \
        libssl-dev \
        uuid-dev \
        # used by librt-engine.so. Technically, only libjson-c3 is needed at runtime
        # but the dev package is needed to build rt-engine
        libjson-c-dev \
        # used by libunilog as a fallback to find glog
        pkg-config \
    # symlink the versioned clang-*-10 executables to clang-*
    && ln -s /usr/bin/clang-format-10 /usr/bin/clang-format \
    && ln -s /usr/bin/clang-tidy-10 /usr/bin/clang-tidy \
    # symlink libjsoncpp to json to maintain include compatibility with Drogon
    && cp -rfs /usr/include/jsoncpp/json/ /usr/include/ \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        python3-pip \
    && python3 -m pip install --upgrade --force-reinstall pip \
    && pip install --no-cache-dir \
        # install these first
        setuptools \
        wheel \
        # the sphinx theme has a bug with docutils>=0.17
        "docutils<0.17" \
    && pip install --no-cache-dir \
        # install testing dependencies
        pytest \
        requests \
        websocket-client \
        # install documentation dependencies
        breathe \
        fastcov \
        sphinx \
        sphinx_copybutton \
        sphinxcontrib-confluencebuilder \
        sphinx-argparse \
        # install linting tool
        black \
        cpplint \
        # install opencv
        opencv-python-headless \
        # install benchmarking dependencies
        pytest-benchmark \
        rich \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# Install XRT and XRM
RUN apt-get update \
    && cd /tmp \
    && wget --progress=dot:mega -O xrt.deb https://www.xilinx.com/bin/public/openDownload?filename=xrt_202110.2.11.648_18.04-amd64-xrt.deb \
    && wget --progress=dot:mega -O xrm.deb https://www.xilinx.com/bin/public/openDownload?filename=xrm_202110.1.2.1539_18.04-x86_64.deb \
    && apt-get update -y \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        ./xrt.deb \
        ./xrm.deb \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && rm -fr /tmp/*

FROM proteus_dev as proteus_dev_vitis_nightly
ARG CACHEBUST=1

RUN mkdir -p /etc/apt/sources.list.d \
    # add both in case one is inaccessible
    && echo "deb [trusted=yes] http://artifactory/artifactory/vitis-ai-deb-master bionic main" >> /etc/apt/sources.list.d/xlnx.list \
    && echo "deb [trusted=yes] http://xcoartifactory/artifactory/vitis-ai-deb-master bionic main" >> /etc/apt/sources.list.d/xlnx.list \
    # force true in case update fails due to one repository being inaccessible
    && apt-get update || true \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        aks \
        libunilog \
        librt-engine \
        libtarget-factory \
        libvart \
        libvitis_ai_library \
        libxir \
    && rm -rf /etc/apt/sources.list.d/xlnx.list \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

FROM proteus_dev as proteus_dev_vitis_stable

RUN apt-get update \
    && cd /tmp \
    && wget --progress=dot:mega -O libunilog.deb https://www.xilinx.com/bin/public/openDownload?filename=libunilog_1.4.0-r75_amd64.deb \
    && wget --progress=dot:mega -O libtarget-factory.deb https://www.xilinx.com/bin/public/openDownload?filename=libtarget-factory_1.4.0-r77_amd64.deb \
    && wget --progress=dot:mega -O libxir.deb https://www.xilinx.com/bin/public/openDownload?filename=libxir_1.4.0-r80_amd64.deb \
    && wget --progress=dot:mega -O libvart.deb https://www.xilinx.com/bin/public/openDownload?filename=libvart_1.4.0-r117_amd64.deb \
    && wget --progress=dot:mega -O libvitis_ai_library.deb https://www.xilinx.com/bin/public/openDownload?filename=libvitis_ai_library_1.4.0-r105_amd64.deb \
    && wget --progress=dot:mega -O librt-engine.deb https://www.xilinx.com/bin/public/openDownload?filename=librt-engine_1.4.0-r178_amd64.deb \
    && wget --progress=dot:mega -O aks.deb https://www.xilinx.com/bin/public/openDownload?filename=aks_1.4.0-r73_amd64.deb \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        ./*.deb \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /tmp/*

FROM proteus_dev_vitis_${NIGHTLY} as proteus_builder

ARG COPY_DIR
ARG PROTEUS_ROOT

# pyinstaller 4.6 has a bug https://github.com/pyinstaller/pyinstaller/issues/6331
RUN pip install --no-cache-dir "pyinstaller!=4.6" \
    # docker-systemctl-replacement v1.5.4504
    && cd /tmp && wget --progress=dot:mega https://github.com/gdraheim/docker-systemctl-replacement/archive/refs/tags/v1.5.4505.tar.gz \
    && tar -xzf v1.5.4505.tar.gz \
    && cd docker-systemctl-replacement-1.5.4505 \
    && pyinstaller files/docker/systemctl3.py --onefile \
    && cp dist/systemctl3 /bin/systemctl \
    && chmod a+x /bin/systemctl \
    && rm -fr /tmp/*

COPY --from=builder ${COPY_DIR} /
COPY . $PROTEUS_ROOT

RUN \
    # make binary for custom script to get FPGAs
    pyinstaller $PROTEUS_ROOT/docker/fpga_util.py --onefile \
    && cp dist/fpga_util /usr/local/bin/fpga-util \
    && chmod a+x /usr/local/bin/fpga-util \
    # create package for the Proteus python library
    && cd ${PROTEUS_ROOT}/src/python \
    && python3 setup.py bdist_wheel

FROM proteus_dev_vitis_${NIGHTLY} as proteus_dev_vitis

ARG COPY_DIR
ARG PROTEUS_ROOT

COPY --from=builder ${COPY_DIR} /
COPY --from=proteus_builder $PROTEUS_ROOT/docker/entrypoint.sh /root/entrypoint.sh
COPY --from=proteus_builder $PROTEUS_ROOT/docker/.bash* /home/proteus-user/
COPY --from=proteus_builder $PROTEUS_ROOT/docker/.env /home/proteus-user/
COPY --from=proteus_builder /usr/local/bin/fpga-util /usr/local/bin/fpga-util
COPY --from=proteus_builder /bin/systemctl /bin/systemctl
COPY --from=proteus_builder ${PROTEUS_ROOT}/src/python/dist/*.whl /tmp/

# run any final commands before finishing the dev image
RUN git lfs install \
    && npm install -g gh-pages \
    && proteus_wheel=$(find /tmp/ -name *.whl 2>/dev/null) \
    && pip install "$proteus_wheel" \
    && rm "$proteus_wheel" \
    && ldconfig

ENTRYPOINT [ "/root/entrypoint.sh" ]
CMD [ "/bin/bash" ]

FROM proteus_dev_vitis_${NIGHTLY} as proteus_builder_2

ARG COPY_DIR
ARG PROTEUS_ROOT

COPY --from=builder ${COPY_DIR} /
# get the systemctl executable
COPY --from=proteus_builder /bin/systemctl /bin/systemctl
COPY . $PROTEUS_ROOT

RUN ldconfig \
    && mkdir ${COPY_DIR} \
    # install libproteus.so
    && cd ${PROTEUS_ROOT} \
    && ./proteus install --all \
    && ./proteus install --get-manifest | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    # build the static GUI files
    && cd src/gui && npm install && npm run build \
    # get all the runtime shared library dependencies for the server
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        # needed for get_dynamic_dependencies.sh
        file \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    # create a copy of all the dynamic libraries needed by the server and workers
    && cd ${PROTEUS_ROOT} \
    && ./docker/get_dynamic_dependencies.sh --copy \
        /usr/local/bin/proteus-server \
        /usr/local/lib/proteus/* \
        ./external/aks/libs/*

FROM proteus_base AS proteus

ARG PROTEUS_ROOT
ARG COPY_DIR
WORKDIR /home/proteus-user

# get all the installed files: the server, workers, and C++ headers
COPY --from=proteus_builder_2 ${COPY_DIR} /
# get the dynamic libraries needed (created by get_dynamic_dependencies.sh)
COPY --from=proteus_builder_2 $PROTEUS_ROOT/deps /
# get AKS kernels
COPY --from=proteus_builder_2 $PROTEUS_ROOT/external/aks/libs/ /opt/xilinx/proteus/aks/libs
# get the static gui files
COPY --from=proteus_builder_2 $PROTEUS_ROOT/src/gui/build/ /opt/xilinx/proteus/gui/
# get the entrypoint script
COPY --from=proteus_builder $PROTEUS_ROOT/docker/entrypoint.sh /root/entrypoint.sh
# get the fpga-util executable
COPY --from=proteus_builder /usr/local/bin/fpga-util /opt/xilinx/proteus/bin/
# get the systemctl executable
COPY --from=proteus_builder /bin/systemctl /bin/systemctl
# get the gosu executable
COPY --from=proteus_builder /usr/local/bin/gosu /usr/local/bin/

# COPY --from=proteus_builder $PROTEUS_ROOT/external/overlaybins /opt/xilinx/overlaybins
COPY --from=proteus_builder_2 $PROTEUS_ROOT/external/aks/graph_zoo /opt/xilinx/proteus/aks/graph_zoo
COPY --from=proteus_builder_2 $PROTEUS_ROOT/external/aks/kernel_zoo /opt/xilinx/proteus/aks/kernel_zoo

ENV LD_LIBRARY_PATH="/usr/local/lib/proteus:/opt/xilinx/proteus/aks"
ENV XILINX_XRT="/opt/xilinx/xrt"
ENV XLNX_VART_FIRMWARE="/opt/xilinx/overlaybins/dpuv3int8"
ENV AKS_ROOT="/opt/xilinx/proteus/aks"
ENV AKS_XMODEL_ROOT="/opt/xilinx/proteus"
ENV PATH="/opt/xilinx/proteus/bin:${PATH}"

# run any final commands before finishing the production image
RUN ldconfig

ENTRYPOINT [ "/root/entrypoint.sh" ]
CMD [ "proteus-server" ]
