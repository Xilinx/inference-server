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

# the base image to use
ARG BASE_IMAGE=ubuntu:18.04
# multi-stage builds place all things to copy in this location between stages
ARG COPY_DIR=/root/deps
# store install manifests for all built packages here for easy reference
ARG MANIFESTS_DIR=${COPY_DIR}/usr/local/manifests
# the working directory is mounted here. Note, this assumption is made in other
# files as well so just changing this value may not work
ARG PROTEUS_ROOT=/workspace/proteus
# the user and group to create in the image. Note, these names are hard-coded
# in other files as well so just changing this value may not work
ARG GNAME=proteus
ARG UNAME=proteus-user

# this image is used as the base to build the inference server for the
# production image. By default, the dev image created with this Dockerfile is
# used
ARG DEV_BASE_IMAGE=${DEV_BASE_IMAGE:-dev}
# specify which platforms the image should be built for. Note, ARM support is
# experimental
ARG TARGETPLATFORM=${TARGETPLATFORM:-linux/amd64}
# image type to build. This must be 'dev' or 'prod'
ARG IMAGE_TYPE=${IMAGE_TYPE:-dev}

# enable building platforms. By default, all platforms are opt-in
ARG ENABLE_VITIS=${ENABLE_VITIS:-no}
ARG ENABLE_TFZENDNN=${ENABLE_TFZENDNN:-no}
ARG TFZENDNN_PATH
ARG ENABLE_PTZENDNN=${ENABLE_PTZENDNN:-no}
ARG PTZENDNN_PATH
ARG ENABLE_MIGRAPHX=${ENABLE_MIGRAPHX:-no}

# this stage installs basic packages used by all images. It's used as an
# ancestor for all subsequent stages
FROM ${BASE_IMAGE} AS base

ARG UNAME
ARG GNAME
ARG PROTEUS_ROOT

ARG UID=1000
ARG GID=1000

ENV TZ=America/Los_Angeles
ENV LANG=en_US.UTF-8
ENV PROTEUS_ROOT=$PROTEUS_ROOT

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        locales \
        sudo \
        tzdata \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    # set up timezone
    && ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone \
    && dpkg-reconfigure --frontend noninteractive tzdata \
    # set up locale
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8 \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# add a user
RUN groupadd -g $GID -o $GNAME \
    && useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME \
    && passwd -d $UNAME \
    && usermod -aG sudo $UNAME \
    && echo 'ALL ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers \
    && usermod -aG $GNAME root

# this stage adds development tools such as compilers to the base image. It's
# used as an ancestor for all development-related stages
FROM base AS dev_base

ARG TARGETPLATFORM
SHELL ["/bin/bash", "-c"]

# install a newer compiler for all development images
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        ca-certificates \
        gcc \
        git \
        make \
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
    && apt-get -y purge --auto-remove software-properties-common \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# install Cmake 3.21.1
RUN if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \
        archive="cmake-3.22.1-linux-x86_64.tar.gz"; \
    elif [[ ${TARGETPLATFORM} == "linux/arm64" ]]; then \
        archive="cmake-3.22.1-linux-aarch64.tar.gz"; \
    else false; fi; \
    url="https://github.com/Kitware/CMake/releases/download/v3.22.1/${archive}" \
    && cd /tmp/ \
    && wget --quiet ${url} \
    && tar --strip-components=1 -xzf ${archive} -C /usr/local \
    && rm -rf /tmp/*

# this stage builds any common dependencies between the inference server and
# any of the platforms. Using common packages between the two ensures version
# compatibility. Note, some of the apt packages here are also used implicitly
# by subsequent build stages
FROM dev_base AS common_builder

ARG COPY_DIR
ARG MANIFESTS_DIR
WORKDIR /tmp

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

# install protobuf 3.19.4 for gRPC - Used by Vitis AI and onnx
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        autoconf \
        automake \
        checkinstall \
        curl \
        libtool \
        unzip \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && VERSION=3.19.4 \
    && wget --quiet https://github.com/protocolbuffers/protobuf/releases/download/v${VERSION}/protobuf-cpp-${VERSION}.tar.gz \
    && tar -xzf protobuf-cpp-${VERSION}.tar.gz \
    && cd protobuf-${VERSION} \
    && ./autogen.sh \
    && ./configure \
    && make -j$(($(nproc) - 1)) \
    && checkinstall -y --pkgname protobuf --pkgversion ${VERSION} --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L protobuf | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && dpkg -L protobuf > ${MANIFESTS_DIR}/protobuf.txt \
    && rm -rf /tmp/*

# install pybind11 2.9.1 - used by Vitis AI
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        python3-dev \
    && VERSION=2.9.1 \
    && wget https://github.com/pybind/pybind11/archive/refs/tags/v${VERSION}.tar.gz \
    && tar -xzf v${VERSION}.tar.gz \
    && cd pybind11-${VERSION}/ \
    && mkdir build \
    && cd build \
    && cmake -DPYBIND11_TEST=OFF .. \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/pybind11.txt \
    && cd /tmp \
    && rm -fr /tmp/*

# install other apt packages used by build stages
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        python3-pip \
        python3-setuptools \
        python3-wheel

# this stage builds all the dependencies of the inference server to be copied
# over
FROM common_builder AS builder

ARG COPY_DIR
ARG MANIFESTS_DIR
ARG TARGETPLATFORM

WORKDIR /tmp
SHELL ["/bin/bash", "-c"]

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

# install gosu 1.12 for dropping down to the user in the entrypoint
RUN if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \
        url="https://github.com/tianon/gosu/releases/download/1.12/gosu-amd64"; \
    elif [[ ${TARGETPLATFORM} == "linux/arm64" ]]; then \
        url="https://github.com/tianon/gosu/releases/download/1.12/gosu-arm64"; \
    else false; fi; \
    wget -O gosu --progress=dot:mega ${url} \
    && chmod 755 gosu \
    && mkdir -p ${COPY_DIR}/usr/local/bin/ && cp gosu ${COPY_DIR}/usr/local/bin/ \
    && rm -rf /tmp/*

# install git-lfs 2.13.3 for managing large files
RUN if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \
        archive="git-lfs-linux-amd64-v2.13.3.tar.gz"; \
    elif [[ ${TARGETPLATFORM} == "linux/arm64" ]]; then \
        archive="git-lfs-linux-arm64-v2.13.3.tar.gz"; \
    else false; fi; \
    url="https://github.com/git-lfs/git-lfs/releases/download/v2.13.3/${archive}" \
    && wget --quiet ${url} \
    && mkdir git-lfs \
    && tar -xzf ${archive} -C git-lfs \
    && mkdir -p ${COPY_DIR}/usr/local/bin/ && cp git-lfs/git-lfs ${COPY_DIR}/usr/local/bin/ \
    && rm -rf /tmp/*

# install lcov 1.15 for test coverage measurement
RUN wget --quiet https://github.com/linux-test-project/lcov/releases/download/v1.15/lcov-1.15.tar.gz \
    && tar -xzf lcov-1.15.tar.gz \
    && cd lcov-1.15 \
    && checkinstall -y --pkgname lcov --pkgversion 1.15 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L lcov | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && dpkg -L lcov > ${MANIFESTS_DIR}/lcov.txt \
    && rm -rf /tmp/*

# install NodeJS 14.16.0 for web gui development
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        xz-utils \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \
        archive="node-v14.16.0-linux-x64.tar.xz"; \
    elif [[ ${TARGETPLATFORM} == "linux/arm64" ]]; then \
        archive="node-v14.16.0-linux-arm64.tar.xz"; \
    else false; fi; \
    url="https://nodejs.org/dist/v14.16.0/${archive}" \
    && wget --quiet ${url} \
    && tar --strip-components=1 -xf ${archive} -C /usr/local \
    && tar --strip-components=1 -xf ${archive} -C ${COPY_DIR}/usr/local \
    # strip the leading directory and add /usr/local/
    && ar -tf ${archive} | sed 's,^[^/]*/,/usr/local/,' > ${MANIFESTS_DIR}/nodejs.txt \
    && rm -rf /tmp/*

# install cxxopts 2.2.1 for argument parsing
RUN wget --quiet https://github.com/jarro2783/cxxopts/archive/refs/tags/v2.2.1.tar.gz \
    && tar -xzf v2.2.1.tar.gz \
    && cd cxxopts-2.2.1 \
    && mkdir -p ${COPY_DIR}/usr/local/include/cxxopts \
    && cp include/cxxopts.hpp ${COPY_DIR}/usr/local/include/cxxopts \
    && rm -rf /tmp/*

# install concurrentqueue 1.0.3 for a lock-free multi-producer and consumer queue
RUN wget --quiet https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.3.tar.gz \
    && tar -xzf v1.0.3.tar.gz \
    && cd concurrentqueue-1.0.3 \
    && cmake . \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/concurrentqueue.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install drogon 1.7.5 for a http server
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        libbrotli-dev \
        libjsoncpp-dev \
        libc-ares-dev \
        libssl-dev \
        uuid-dev \
        zlib1g-dev \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    # symlink libjsoncpp to json to maintain include compatibility with Drogon
    && cp -rfs /usr/include/jsoncpp/json/ /usr/include/ \
    # install drogon
    && cd /tmp && git clone -b v1.7.5 https://github.com/an-tao/drogon \
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
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/drogon.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install libb64 2.0.0.1. The default version in apt adds linebreaks when encoding
RUN wget --quiet https://github.com/libb64/libb64/archive/refs/tags/v2.0.0.1.tar.gz \
    && tar -xzf v2.0.0.1.tar.gz \
    && cd libb64-2.0.0.1 \
    && make -j$(($(nproc) - 1)) all_src \
    && mkdir -p ${COPY_DIR}/usr/local/lib && cp src/libb64.a ${COPY_DIR}/usr/local/lib \
    && mkdir -p ${COPY_DIR}/usr/local/include && cp -r include/b64 ${COPY_DIR}/usr/local/include \
    && rm -rf /tmp/*

# install GTest 1.11.0 for C++ testing
RUN wget --quiet https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz \
    && tar -xzf release-1.11.0.tar.gz \
    && cd googletest-release-1.11.0 \
    && mkdir -p build && cd build \
    && cmake .. \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/gtest.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install FFmpeg 3.4.8 for opencv
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        nasm \
    && wget --quiet https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n3.4.8.tar.gz \
    && tar -xzf n3.4.8.tar.gz \
    && cd FFmpeg-n3.4.8 \
    && ./configure --disable-static --enable-shared --disable-doc \
    && make -j$(($(nproc) - 1)) \
    && checkinstall -y --pkgname ffmpeg --pkgversion 3.4.8 --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L ffmpeg | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && dpkg -L ffmpeg > ${MANIFESTS_DIR}/ffmpeg.txt \
    && rm -rf /tmp/*

# install opencv 3.4.3 for image and video processing
# pkg-config is needed to find ffmpeg
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        pkg-config \
    && wget --quiet https://github.com/opencv/opencv/archive/3.4.3.tar.gz \
    && tar -xzf 3.4.3.tar.gz \
    && cd opencv-3.4.3 \
    && mkdir -p build \
    && cd build \
    && cmake -DBUILD_SHARED_LIBS=ON -DBUILD_TEST=OFF -DBUILD_PERF_TESTS=OFF -DWITH_FFMPEG=ON .. \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/opencv.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install spdlog 1.8.2
RUN wget --quiet https://github.com/gabime/spdlog/archive/refs/tags/v1.8.2.tar.gz \
    && tar -xzf v1.8.2.tar.gz \
    && cd spdlog-1.8.2 \
    && mkdir -p build && cd build \
    && cmake .. \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/spdlog.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install prometheus-cpp 0.12.2 for metrics
RUN wget --quiet https://github.com/jupp0r/prometheus-cpp/archive/refs/tags/v0.12.2.tar.gz \
    && tar -xzf v0.12.2.tar.gz \
    && cd prometheus-cpp-0.12.2 \
    && mkdir -p build && cd build \
    && cmake .. \
        -DENABLE_PUSH=OFF \
        -DENABLE_PULL=OFF \
        -DENABLE_TESTING=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DUSE_THIRDPARTY_LIBRARIES=OFF \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/prometheus-cpp.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# get Nlohmann JSON for building opentelemetry
RUN wget --quiet https://github.com/nlohmann/json/archive/refs/tags/v3.7.3.tar.gz \
    && tar -xzf v3.7.3.tar.gz \
    && cd json-3.7.3 \
    && mkdir -p build && cd build \
    && cmake .. \
        -DBUILD_TESTING=OFF \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && rm -rf /tmp/*

# install Apache Thrift 0.12.0 for running the jaeger exporter in opentelemetry
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        bison \
        flex \
        libboost-all-dev \
    && VERSION=0.15.0 \
    && cd /tmp && wget --quiet https://github.com/apache/thrift/archive/refs/tags/v${VERSION}.tar.gz \
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
    && make -j$(($(nproc) - 1)) \
    && make install \
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
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/opentelemetry.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install wrk for http benchmarking
RUN wget --quiet https://github.com/wg/wrk/archive/refs/tags/4.1.0.tar.gz \
    && tar -xzf 4.1.0.tar.gz \
    && cd wrk-4.1.0 \
    && make -j$(($(nproc) - 1)) \
    && mkdir -p ${COPY_DIR}/usr/local/bin && cp wrk ${COPY_DIR}/usr/local/bin \
    && rm -rf /tmp/*

# install include-what-you-use 0.14
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        libclang-10-dev \
        clang-10 \
        llvm-10-dev \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && wget --quiet https://github.com/include-what-you-use/include-what-you-use/archive/refs/tags/0.14.tar.gz \
    && tar -xzf 0.14.tar.gz \
    && cd include-what-you-use-0.14 \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_PREFIX_PATH=/usr/lib/llvm-10 .. \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/include_what_you_use.txt \
    && cd /tmp \
    && rm -fr /tmp/*

# install gRPC 1.44.0
RUN git clone --depth=1 --branch v1.44.0 --single-branch https://github.com/grpc/grpc \
    && cd grpc \
    && git submodule update --init third_party/abseil-cpp third_party/re2 \
    && mkdir -p build && cd build \
    && cmake -DgRPC_ZLIB_PROVIDER=package \
        -DgRPC_CARES_PROVIDER=custom \
        -D_gRPC_CARES_LIBRARIES=cares \
        -DgRPC_SSL_PROVIDER=package \
        -DgRPC_RE2_PROVIDER=module \
        -DgRPC_PROTOBUF_PROVIDER=package \
        -DgRPC_PROTOBUF_PACKAGE_TYPE="" \
        -DgRPC_BUILD_TESTS=OFF \
        -DgRPC_ABSL_PROVIDER=module \
        .. \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/grpc.txt \
    && cd /tmp \
    && rm -fr /tmp/*

# install efsw for directory monitoring
RUN git clone https://github.com/SpartanJ/efsw.git \
    && cd efsw && mkdir build && cd build \
    && cmake .. \
    && make -j \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/efsw.txt \
    && cd /tmp && rm -fr /tmp/*

# install doxygen 1.9.2
# RUN cd /tmp && wget --quiet https://github.com/doxygen/doxygen/archive/refs/tags/Release_1_9_2.tar.gz \
#     && tar -xzf Release_1_9_2.tar.gz \
#     && cd doxygen-Release_1_9_2/ \
#     && mkdir -p build \
#     && cd build \
#     && cmake ..
#     && make -j$(($(nproc) - 1)) \
#     && make install \
#     && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
#     && cd /tmp \
#     && rm -fr /tmp/*

#? This no longer seems needed but is kept around in case
# Delete /usr/local/man which is a symlink and cannot be copied later by BuildKit.
# Note: this works without BuildKit: https://github.com/docker/buildx/issues/150
# RUN cp -rf ${COPY_DIR}/usr/local/man/ ${COPY_DIR}/usr/local/share/man/ \
#     && rm -rf ${COPY_DIR}/usr/local/man/

FROM common_builder AS vitis_builder

WORKDIR /tmp
SHELL ["/bin/bash", "-c"]
ARG COPY_DIR
ARG MANIFESTS_DIR
ARG TARGETPLATFORM

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

# install json-c 0.15 for Vitis AI runtime
RUN wget --quiet https://github.com/json-c/json-c/archive/refs/tags/json-c-0.15-20200726.tar.gz \
    && tar -xzf json-c-0.15-20200726.tar.gz \
    && cd json-c-json-c-0.15-20200726 \
    && mkdir build \
    && cd build \
    && cmake .. \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_STATIC_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/json-c.txt

# Install XRT and XRM
RUN apt-get update \
    && if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \
        cd /tmp \
        && wget --quiet -O xrt.deb https://www.xilinx.com/bin/public/openDownload?filename=xrt_202120.2.12.427_18.04-amd64-xrt.deb \
        && wget --quiet -O xrm.deb https://www.xilinx.com/bin/public/openDownload?filename=xrm_202120.1.3.29_18.04-x86_64.deb \
        && apt-get update -y \
        && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
            ./xrt.deb \
            ./xrm.deb \
        # copy over debians to COPY_DIR so we can install them as debians in
        # the final image. In particular, the same XRT needs to also be
        # installed on the host and so a .deb allows for easy version control
        && cp ./xrt.deb ${COPY_DIR} \
        && cp ./xrm.deb ${COPY_DIR}; \
    fi;

# Install Vitis AI runtime and build dependencies
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        libgoogle-glog-dev \
        libssl-dev \
        pkg-config \
    && git clone --recursive --single-branch --branch v2.5 --depth 1 https://github.com/Xilinx/Vitis-AI.git \
    && export VITIS_ROOT=/tmp/Vitis-AI/src/Vitis-AI-Runtime/VART \
    && git clone --single-branch -b v2.0 --depth 1 https://github.com/Xilinx/rt-engine.git ${VITIS_ROOT}/rt-engine; \
    cd ${VITIS_ROOT}/unilog \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build \
    && cd ./build && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/unilog.txt \
    && cd ${VITIS_ROOT}/xir \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build --build-python \
    && cd ./build && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/xir.txt \
    && cd ${VITIS_ROOT}/target_factory \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build \
    && cd ./build && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/target_factory.txt \
    && cd ${VITIS_ROOT}/vart \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --cmake-options="-DBUILD_TEST=OFF" --build-python --build-dir ./build \
    && cd ./build && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/vart.txt \
    && cd ${VITIS_ROOT}/rt-engine \
    && ./cmake.sh --clean --build-dir=./build --type=release --cmake-options="-DXRM_DIR=/opt/xilinx/xrm/share/cmake" --cmake-options="-DBUILD_TESTS=OFF" --install-prefix /usr/local/ \
    && cd ./build && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/rt-engine.txt \
    && cd /tmp/Vitis-AI/src/AKS \
    # fix bug in AKS
    && sed -i '46i _global = nullptr;' ./src/AksTopContainer.cpp \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build \
    && cd ./build && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/aks.txt

FROM dev_base AS vitis_installer_no

FROM dev_base AS vitis_installer_yes

ARG COPY_DIR

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        # used by vitis ai runtime
        libgoogle-glog-dev \
        libboost-filesystem1.65.1 \
        libboost-system1.65.1 \
        libboost-serialization1.65.1 \
        libboost-thread1.65.1 \
        libboost1.65-dev \
        # used to detect if ports are in use for XRM
        net-tools \
        # used by libunilog as a fallback to find glog
        pkg-config \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

COPY --from=vitis_builder ${COPY_DIR} /

# install all the Vitis AI runtime libraries built from source as debians
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        /*.deb \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && rm /*.deb

FROM common_builder AS tfzendnn_builder

ARG COPY_DIR
ARG MANIFESTS_DIR
ARG TFZENDNN_PATH
WORKDIR /tmp

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

COPY $TFZENDNN_PATH /tmp/

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        zip unzip \
    # Check to verify if package is TF+ZenDNN
    && echo "51b3b4093775ff2b67e06f18d01b41ac  $(basename $TFZENDNN_PATH)" | md5sum -c - \
    && unzip $(basename $TFZENDNN_PATH) \
    && cd $(basename ${TFZENDNN_PATH%.*}) \
    # To avoid protobuf version issues, create subfolder and copy include files
    && mkdir -p ${COPY_DIR}/usr/include/tfzendnn/ \
    && mkdir -p ${COPY_DIR}/usr/lib \
    # copy and list files that are copied
    && cp -rv include/* ${COPY_DIR}/usr/include/tfzendnn | cut -d"'" -f 4 > ${MANIFESTS_DIR}/tfzendnn.txt \
    && cp -rv lib/*.so* ${COPY_DIR}/usr/lib | cut -d"'" -f 4 >> ${MANIFESTS_DIR}/tfzendnn.txt

FROM vitis_installer_${ENABLE_VITIS} AS tfzendnn_installer_no

FROM vitis_installer_${ENABLE_VITIS} AS tfzendnn_installer_yes

COPY --from=tfzendnn_builder ${COPY_DIR} /

FROM common_builder AS ptzendnn_builder

ARG COPY_DIR
ARG MANIFESTS_DIR
ARG PTZENDNN_PATH
WORKDIR /tmp

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

COPY $PTZENDNN_PATH /tmp/

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        zip \
        unzip \
    && cd /tmp/ \
    # Check to verify if package is PT+ZenDNN
    && echo "a191f2305f1cae6e00c82a1071df9708  $(basename $PTZENDNN_PATH)" | md5sum -c - \
    && unzip $(basename $PTZENDNN_PATH) \
    && cd $(basename ${PTZENDNN_PATH%.*}) \
    # To avoid protobuf version issues, create subfolder and copy include files
    && mkdir -p ${COPY_DIR}/usr/include/ptzendnn/ \
    && mkdir -p ${COPY_DIR}/usr/lib \
    # copy and list files that are copied
    && cp -rv include/* ${COPY_DIR}/usr/include/ptzendnn | cut -d"'" -f 4 > ${MANIFESTS_DIR}/ptzendnn.txt \
    && cp -rv lib/*.so* ${COPY_DIR}/usr/lib | cut -d"'" -f 4 >> ${MANIFESTS_DIR}/ptzendnn.txt

# build jemalloc 5.3.0. Build uses autoconf and checkinstall implicitly
RUN VERSION=5.3.0 \
    && wget -q https://github.com/jemalloc/jemalloc/archive/refs/tags/${VERSION}.tar.gz \
    && tar -xzf ${VERSION}.tar.gz \
    && cd jemalloc-${VERSION} && ./autogen.sh \
    && make -j \
    && checkinstall -y --pkgname jemalloc --pkgversion ${VERSION} --pkgrelease 1 make install \
    && cd /tmp \
    && dpkg -L jemalloc | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && dpkg -L jemalloc > ${MANIFESTS_DIR}/jemalloc.txt

FROM tfzendnn_installer_${ENABLE_TFZENDNN} AS ptzendnn_installer_no

FROM tfzendnn_installer_${ENABLE_TFZENDNN} AS ptzendnn_installer_yes

COPY --from=ptzendnn_builder ${COPY_DIR} /

FROM common_builder AS migraphx_builder

ARG COPY_DIR
ARG MANIFESTS_DIR
WORKDIR /tmp

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

# Install migraphx which supports GPU targets.  At the time of writing this,
# it was necessary to build migraphx from source because the release branch
# didn't contain needed headers but it should be possible to install from repo
# some day.

# Install dependencies for migraphx
# ENV PYTHONPATH=/opt/rocm/lib   # This addition may be needed to import migraphx in Python scripts
#        for Release version, since it's set up differently than dev

# The following apt packages are also required to install rocm/MIGraphX but have already
# been installed by this Dockerfile:
# python3-dev, build-essential, git, sudo, python3-pip

# Install rbuild
RUN echo "deb [arch=amd64 trusted=yes] http://repo.radeon.com/rocm/apt/5.0/ ubuntu main" > /etc/apt/sources.list.d/rocm.list \
    && apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends\
        aria2 \
        half \
        libnuma-dev \
        libpython3.6-dev \
        miopen-hip-dev \
        rocblas-dev \
        rocm-cmake \
        rocm-dev \
    && ln -s /opt/rocm-* /opt/rocm \
    && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \
    && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \
    && ldconfig \
    && pip3 install --no-cache-dir https://github.com/RadeonOpenCompute/rbuild/archive/f74d130aac0405c7e6bc759d331f913a7577bd54.tar.gz

# Install MIGraphX from source
RUN mkdir -p /migraphx \
    && cd /migraphx && git clone --branch develop https://github.com/ROCmSoftwarePlatform/AMDMIGraphX src \
    && cd /migraphx/src  && git checkout cb18b0b5722373c49f5c257380af206e13344735 \
    # disable building documentation and tests. Is there a better way?
    && sed -i 's/^add_subdirectory(doc)/#&/' CMakeLists.txt \
    && sed -i 's/^add_subdirectory(test)/#&/' CMakeLists.txt \
    && rbuild package -d /migraphx/deps -B /migraphx/build --define "BUILD_TESTING=OFF" \
    && cp /migraphx/build/*.deb ${COPY_DIR}

# build wheel for onnx (it needs protobuf), used by migraphx example scripts
RUN pip3 wheel onnx \
    && cp *.whl ${COPY_DIR}

FROM ptzendnn_installer_${ENABLE_PTZENDNN} AS migraphx_installer_no

FROM ptzendnn_installer_${ENABLE_PTZENDNN} AS migraphx_installer_yes

ARG COPY_DIR

COPY --from=migraphx_builder ${COPY_DIR} /

# install all .deb files from the builder
RUN echo "deb [arch=amd64 trusted=yes] http://repo.radeon.com/rocm/apt/5.0/ ubuntu main" > /etc/apt/sources.list.d/rocm.list \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        python3-pip \
        rsync \
        # these packages are required to build with migraphx but don't get installed
        # as dependencies. They also create /opt/rocm-* so we can make a symlink
        # before installing migraphx
        miopen-hip-dev \
        rocm-device-libs \
        rocblas-dev \
        libnuma1 \
    && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \
    && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \
    && ldconfig \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        /*.deb \
    # having symlinks between rocm-* and rocm complicates building the production
    # image so move files over but keep the symlink for compatibility
    && rsync -a /opt/rocm/* /opt/rocm-* \
    && rm -rf /opt/rocm \
    && ln -s /opt/rocm-* /opt/rocm \
    # install .whl from the builder
    && pip3 install --no-cache-dir /*.whl \
    && rm -f /*.whl \
    # clean up
    && apt-get purge -y --auto-remove  python3-pip rsync \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && rm -f /*.deb

FROM common_builder AS builder_dev

ARG COPY_DIR
ARG PROTEUS_ROOT
ARG ENABLE_VITIS
SHELL ["/bin/bash", "-c"]

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR}

# pyinstaller 4.6 has a bug https://github.com/pyinstaller/pyinstaller/issues/6331
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        zlib1g-dev \
    && pip3 install --no-cache-dir "pyinstaller!=4.6" \
    # docker-systemctl-replacement v1.5.4504
    && cd /tmp && wget --quiet https://github.com/gdraheim/docker-systemctl-replacement/archive/refs/tags/v1.5.4505.tar.gz \
    && tar -xzf v1.5.4505.tar.gz \
    && cd docker-systemctl-replacement-1.5.4505 \
    && pyinstaller files/docker/systemctl3.py --onefile \
    && chmod a+x dist/systemctl3 \
    && mkdir -p ${COPY_DIR}/bin/  \
    && cp dist/systemctl3 ${COPY_DIR}/bin/systemctl \
    && rm -fr /tmp/*

COPY . $PROTEUS_ROOT

RUN if [[ ${ENABLE_VITIS} == "yes" ]]; then \
        # make binary for custom script to get FPGAs
        pyinstaller $PROTEUS_ROOT/docker/fpga_util.py --onefile \
        && chmod a+x dist/fpga_util \
        && mkdir -p ${COPY_DIR}/usr/local/bin/ \
        && cp dist/fpga_util ${COPY_DIR}/usr/local/bin/fpga-util; \
    fi

FROM migraphx_installer_${ENABLE_MIGRAPHX} AS dev

ARG COPY_DIR
ARG PROTEUS_ROOT
ARG UNAME
ARG TARGETPLATFORM
SHELL ["/bin/bash", "-c"]

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        # used for auto-completing bash commands
        bash-completion \
        curl \
        make \
        # used for git
        openssh-client \
        python3 \
        python3-dev \
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
    # clang-tidy-10 installs pyyaml which can't be uninstalled with pip
    && pip install --no-cache-dir --ignore-installed \
        # install testing dependencies
        pytest \
        pytest-cpp \
        pytest-xprocess \
        requests \
        # install documentation dependencies
        breathe \
        fastcov \
        sphinx \
        sphinx_copybutton \
        sphinxcontrib-confluencebuilder \
        sphinx-argparse \
        sphinx-issues \
        # install linting tools
        black \
        cpplint \
        cmakelang  \
        pre-commit \
        # install opencv
        opencv-python-headless \
        # install benchmarking dependencies
        pytest-benchmark \
        rich \
        # used for Python bindings
        pybind11_mkdoc \
        pybind11-stubgen \
    # clean up
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder ${COPY_DIR} /
COPY --from=common_builder ${COPY_DIR} /
COPY --from=builder_dev ${COPY_DIR} /
COPY --from=builder_dev $PROTEUS_ROOT/docker/entrypoint.sh /root/entrypoint.sh
COPY --from=builder_dev $PROTEUS_ROOT/docker/.bash* /home/${UNAME}/
COPY --from=builder_dev $PROTEUS_ROOT/docker/.env /home/${UNAME}/

# run any final commands before finishing the dev image
RUN git lfs install \
    && npm install -g gh-pages \
    && ldconfig

ENTRYPOINT [ "/root/entrypoint.sh", "user"]
CMD [ "/bin/bash" ]

FROM ${DEV_BASE_IMAGE} AS builder_prod

ARG COPY_DIR
ARG MANIFESTS_DIR
ARG PROTEUS_ROOT
ARG ENABLE_VITIS
ARG ENABLE_MIGRAPHX

COPY . $PROTEUS_ROOT

RUN ldconfig \
    # delete any inherited artifacts and recreate
    && rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR} \
    # install libproteus.so
    && cd ${PROTEUS_ROOT} \
    && ./proteus install --all \
    && ./proteus install --get-manifest | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && ./proteus install --get-manifest > ${MANIFESTS_DIR}/proteus.txt \
    # build the static GUI files
    # && cd src/gui && npm install && npm run build \
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
    && ./docker/get_dynamic_dependencies.sh --vitis ${ENABLE_VITIS} --migraphx ${ENABLE_MIGRAPHX} > ${MANIFESTS_DIR}/prod.txt \
    && ./docker/get_dynamic_dependencies.sh --copy ${COPY_DIR} --vitis ${ENABLE_VITIS} --migraphx ${ENABLE_MIGRAPHX}

FROM base AS vitis_installer_prod_yes

ARG COPY_DIR
ARG PROTEUS_ROOT

# get AKS kernels
COPY --from=builder_prod $PROTEUS_ROOT/external/aks/libs/ /opt/xilinx/proteus/aks/libs/
# get the fpga-util executable
COPY --from=builder_prod /usr/local/bin/fpga-util /opt/xilinx/proteus/bin/

# we need the xclbins in the image and they must be copied from a path local
# to the build tree. But we also need this hack so the copy doesn't fail
# if this directory doesn't exist
COPY --from=builder_prod $PROTEUS_ROOT/docker/.env $PROTEUS_ROOT/external/overlaybin[s]/ /opt/xilinx/overlaybins/

# get the pre-defined AKS graphs and kernels
COPY --from=builder_prod $PROTEUS_ROOT/external/aks/graph_zoo/ /opt/xilinx/proteus/aks/graph_zoo/
COPY --from=builder_prod $PROTEUS_ROOT/external/aks/kernel_zoo/ /opt/xilinx/proteus/aks/kernel_zoo/

ENV LD_LIBRARY_PATH="/opt/xilinx/proteus/aks"
ENV XILINX_XRT="/opt/xilinx/xrt"
# TODO(varunsh): we shouldn't hardcode dpuv3int8 here
ENV XLNX_VART_FIRMWARE="/opt/xilinx/overlaybins/dpuv3int8"
ENV AKS_ROOT="/opt/xilinx/proteus/aks"
ENV AKS_XMODEL_ROOT="/opt/xilinx/proteus"
ENV PATH="/opt/xilinx/proteus/bin:${PATH}"

FROM base AS vitis_installer_prod_no

FROM vitis_installer_prod_${ENABLE_VITIS} AS prod

ARG PROTEUS_ROOT
ARG COPY_DIR
ARG UNAME
WORKDIR /home/${UNAME}

# get all the installed files: the server, workers, C++ headers and dependencies
COPY --from=builder_prod ${COPY_DIR} /

# get the static gui files
# COPY --from=builder_prod $PROTEUS_ROOT/src/gui/build/ /opt/xilinx/proteus/gui/
# get the entrypoint script
COPY --from=builder_prod $PROTEUS_ROOT/docker/entrypoint.sh /root/entrypoint.sh
# get the systemctl executable - pulled in by get_dynamic_dependencies.sh
# COPY --from=builder_dev ${COPY_DIR}/bin/systemctl /bin/systemctl
# get the gosu executable
COPY --from=builder_prod /usr/local/bin/gosu /usr/local/bin/
# get the .bashrc and .env to configure the environment for all shells
COPY --from=builder_prod $PROTEUS_ROOT/docker/.bash* $PROTEUS_ROOT/docker/.env /home/${UNAME}/
COPY --from=builder_prod $PROTEUS_ROOT/docker/.root_bashrc /root/.bashrc
COPY --from=builder_prod $PROTEUS_ROOT/docker/.env /root/

# run any final commands before finishing the production image
RUN echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \
    && ldconfig

# we need to run as root because KServe mounts models to /mnt/models which means
# the server needs root access to access the mounted assets
ENTRYPOINT [ "/root/entrypoint.sh", "root" ]
CMD [ "proteus-server" ]

FROM ${IMAGE_TYPE} AS final

ARG ENABLE_VITIS
ARG ENABLE_TFZENDNN
ARG ENABLE_PTZENDNN
ARG ENABLE_MIGRAPHX

LABEL project="proteus"
LABEL vitis=${ENABLE_VITIS}
LABEL tfzendnn=${ENABLE_TFZENDNN}
LABEL ptzendnn=${ENABLE_PTZENDNN}
LABEL migraphx=${ENABLE_MIGRAPHX}
