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

# the base image to use
ARG BASE_IMAGE=$[BASE_IMAGE]
# multi-stage builds place all things to copy in this location between stages
ARG COPY_DIR=/root/deps
# store install manifests for all built packages here for easy reference
ARG MANIFESTS_DIR=${COPY_DIR}/usr/local/share/manifests
# the working directory is mounted here. Note, this assumption is made in other
# files as well so just changing this value may not work
ARG AMDINFER_ROOT=/workspace/amdinfer
# the user and group to create in the image. Note, these names are hard-coded
# in other files as well so just changing this value may not work
ARG GNAME=amdinfer
ARG UNAME=amdinfer-user

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

# this stage adds development tools such as compilers to the base image. It's
# used as an ancestor for all development-related stages
FROM ${BASE_IMAGE} AS dev_base

ARG TARGETPLATFORM
SHELL ["/bin/bash", "-c"]

# install common dev tools
$[ADD_DEV_TOOLS]

# add a newer compiler if required
$[ADD_COMPILER]

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
# compatibility. Note, some of the packages here are also used implicitly
# by subsequent build stages
FROM dev_base AS common_builder

ARG COPY_DIR
ARG MANIFESTS_DIR
WORKDIR /tmp

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

# install other distro packages used by build stages
$[INSTALL_BUILD_PACKAGES]

# install extra optional distro packages
$[INSTALL_OPTIONAL_BUILD_PACKAGES]

# install Boost for VAI and Apache Thrift. This must match the boost version installed by XRT package
RUN wget --quiet https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.gz \
    && tar -xzf boost_1_71_0.tar.gz \
    && cd boost_1_71_0 \
    # && PYTHON_ROOT=$(dirname $(find /usr/include -name pyconfig.h)) \
    # && ./bootstrap.sh --with-python=/usr/bin/python3 --with-python-root=${PYTHON_ROOT} \
    && INSTALL_DIR=/tmp/installed \
    && mkdir -p ${INSTALL_DIR} \
    && ./bootstrap.sh --without-libraries=python --prefix=${INSTALL_DIR} \
    && ./b2 install -j $(($(nproc) - 1)) \
    && find ${INSTALL_DIR} -type f -o -type l | sed 's/\/tmp\/installed/\/usr\/local/' > ${MANIFESTS_DIR}/boost.txt \
    # && CPLUS_INCLUDE_PATH=${PYTHON_ROOT} ./b2 install --with=all -j $(($(nproc) - 1)) \
    && cp -rf ${INSTALL_DIR}/* /usr/local \
    && cat ${MANIFESTS_DIR}/boost.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cd /tmp \
    && rm -rf /tmp/*

# install protobuf 3.19.4 for gRPC - Used by Vitis AI and onnx
# onnx requires 3.19.4 due to Protobuf python
RUN VERSION=3.19.4 \
    && wget --quiet https://github.com/protocolbuffers/protobuf/releases/download/v${VERSION}/protobuf-cpp-${VERSION}.tar.gz \
    && tar -xzf protobuf-cpp-${VERSION}.tar.gz \
    && cd protobuf-${VERSION}/cmake \
    && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=NO -DBUILD_SHARED_LIBS=YES \
    && cmake --build build --target install -- -j$(($(nproc) - 1)) \
    && cat build/install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cp build/install_manifest.txt ${MANIFESTS_DIR}/protobuf.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install pybind11 2.9.1 - used by Vitis AI
RUN VERSION=2.9.1 \
    && wget https://github.com/pybind/pybind11/archive/refs/tags/v${VERSION}.tar.gz \
    && tar -xzf v${VERSION}.tar.gz \
    && cd pybind11-${VERSION}/ \
    && mkdir build \
    && cd build \
    && cmake -DPYBIND11_TEST=OFF .. \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cp install_manifest.txt ${MANIFESTS_DIR}/pybind11.txt \
    && cd /tmp \
    && rm -fr /tmp/*

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
    wget -O gosu --quiet ${url} \
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

# install NodeJS 14.16.0 for web gui development
RUN if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \
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

# install jsoncpp for Drogon
RUN VERSION=1.7.4 \
    && wget --quiet https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/${VERSION}.tar.gz \
    && tar -xzf ${VERSION}.tar.gz \
    && cd jsoncpp-${VERSION} \
    && cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_STATIC_LIBS=OFF \
        -DJSONCPP_WITH_TESTS=OFF \
        -DINCLUDE_INSTALL_DIR=/usr/local/include/jsoncpp \
    && cmake --build build --target install -- -j$(($(nproc) - 1)) \
    && cat build/install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cp build/install_manifest.txt ${MANIFESTS_DIR}/jsoncpp.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install c-ares for gRPC and Drogon
RUN VERSION=1_14_0 \
    && wget --quiet https://github.com/c-ares/c-ares/archive/refs/tags/cares-${VERSION}.tar.gz \
    && tar -xzf cares-${VERSION}.tar.gz \
    && cd c-ares-cares-${VERSION} \
    && cmake -S . -B build \
    && cmake --build build --target install -- -j$(($(nproc) - 1)) \
    && cat build/install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cp build/install_manifest.txt ${MANIFESTS_DIR}/c-ares.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install drogon 1.8.1 for a http server
RUN git clone -b v1.8.1 https://github.com/an-tao/drogon \
    && cd drogon \
    && git submodule update --init --recursive \
    && cmake -S . -B build \
        -DBUILD_EXAMPLES=OFF \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_ORM=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_CTL=OFF \
    # work-around trantor bug (https://github.com/an-tao/trantor/issues/229)
    && sed -i '205{h;d};214G' ./trantor/trantor/net/inner/AresResolver.cc \
    && sed -i '205{h;d};214G' ./trantor/trantor/net/inner/AresResolver.cc \
    && sed -i '205{h;d};214G' ./trantor/trantor/net/inner/AresResolver.cc \
    && cmake --build build --target install -- -j$(($(nproc) - 1)) \
    && cat build/install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cp build/install_manifest.txt ${MANIFESTS_DIR}/drogon.txt \
    && cd /tmp \
    && rm -rf /tmp/*

# install libb64 2.0.0.1 - fpic for cibuildwheel
RUN wget --quiet https://github.com/libb64/libb64/archive/refs/tags/v2.0.0.1.tar.gz \
    && tar -xzf v2.0.0.1.tar.gz \
    && cd libb64-2.0.0.1 \
    && CFLAGS="-fpic" make -j$(($(nproc) - 1)) all_src \
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
RUN wget --quiet https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n3.4.8.tar.gz \
    && tar -xzf n3.4.8.tar.gz \
    && cd FFmpeg-n3.4.8 \
    && ./configure --disable-static --enable-shared --disable-doc \
    && make -j$(($(nproc) - 1)) \
    && INSTALL_DIR=/tmp/installed \
    && mkdir -p ${INSTALL_DIR} \
    && make install DESTDIR=${INSTALL_DIR} \
    && find ${INSTALL_DIR} -type f -o -type l | sed 's/\/tmp\/installed//' > ${MANIFESTS_DIR}/ffmpeg.txt \
    && cp -rP ${INSTALL_DIR}/* / \
    && cat ${MANIFESTS_DIR}/ffmpeg.txt | xargs -i bash -c "cp --parents -P {} ${COPY_DIR}" \
    && cd /tmp \
    && rm -rf /tmp/*

# install opencv 3.4.3 for image and video processing
# pkg-config is needed to find ffmpeg
RUN wget --quiet https://github.com/opencv/opencv/archive/3.4.3.tar.gz \
    && tar -xzf 3.4.3.tar.gz \
    && cd opencv-3.4.3 \
    && mkdir -p build \
    && cd build \
    && cmake -DBUILD_SHARED_LIBS=ON -DBUILD_TEST=OFF -DBUILD_PERF_TESTS=OFF -DWITH_FFMPEG=ON .. \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cp install_manifest.txt ${MANIFESTS_DIR}/opencv.txt \
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
RUN VERSION=0.15.0 \
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
RUN VERSION=1.1.0 \
    && wget --quiet https://github.com/open-telemetry/opentelemetry-cpp/archive/refs/tags/v${VERSION}.tar.gz \
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
    && cp install_manifest.txt ${MANIFESTS_DIR}/opentelemetry.txt \
    && cd /tmp \
    && rm -rf /tmp/*

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
RUN COMMIT=6b51944994b5c77dbd7edce66846e378a3bf4d8e \
    && wget --quiet https://github.com/SpartanJ/efsw/archive/${COMMIT}.tar.gz \
    && tar -xzf ${COMMIT}.tar.gz \
    && cd efsw-${COMMIT}/ && mkdir build && cd build \
    && cmake .. \
    && make -j \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/efsw.txt \
    && cd /tmp && rm -fr /tmp/*

# install half for FP16 data type
RUN wget -O half_2.2.0.zip https://sourceforge.net/projects/half/files/latest/download \
    && unzip half_2.2.0.zip -d half \
    && mkdir -p ${COPY_DIR}/usr/local/include/half \
    && mv half/include/half.hpp ${COPY_DIR}/usr/local/include/half \
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

$[BUILD_OPTIONAL]

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
$[INSTALL_XRT]

# Install Vitis AI runtime and build dependencies
RUN git clone --recursive --single-branch --branch v2.5 --depth 1 https://github.com/Xilinx/Vitis-AI.git \
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
    # find the required components. Adding this was needed when using Boost from Ubuntu apt repositories
    && sed -i '42i find_package(Boost COMPONENTS system filesystem thread serialization)' ./CMakeLists.txt \
    && ./cmake.sh --clean --build-dir=./build --type=release --cmake-options="-DXRM_DIR=/opt/xilinx/xrm/share/cmake" --cmake-options="-DBUILD_TESTS=OFF" --install-prefix /usr/local/ \
    && cd ./build && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/rt-engine.txt \
    && cd /tmp/Vitis-AI/src/AKS \
    # fix bug in AKS
    && sed -i '46i _global = nullptr;' ./src/AksTopContainer.cpp \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build \
    && cd ./build && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cp install_manifest.txt ${MANIFESTS_DIR}/aks.txt

RUN COMMIT=e5c51b541d5cbcf353d4165499103f5e6d7e7ea9 \
    && wget --quiet https://github.com/fpagliughi/sockpp/archive/${COMMIT}.tar.gz \
    && tar -xzf ${COMMIT}.tar.gz \
    && cd sockpp-${COMMIT}/ \
    && mkdir build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
    && make -j$(($(nproc) - 1)) \
    && make install \
    && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && cat install_manifest.txt > ${MANIFESTS_DIR}/sockcpp.txt

FROM dev_base AS vitis_installer_no

FROM dev_base AS vitis_installer_yes

ARG COPY_DIR

COPY --from=vitis_builder ${COPY_DIR} /

$[INSTALL_VITIS]

FROM common_builder AS tfzendnn_builder

ARG COPY_DIR
ARG MANIFESTS_DIR
WORKDIR /tmp

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

$[BUILD_TFZENDNN]

FROM vitis_installer_${ENABLE_VITIS} AS tfzendnn_installer_no

FROM vitis_installer_${ENABLE_VITIS} AS tfzendnn_installer_yes

ARG COPY_DIR

COPY --from=tfzendnn_builder ${COPY_DIR} /

FROM common_builder AS ptzendnn_builder

ARG COPY_DIR
ARG MANIFESTS_DIR
WORKDIR /tmp

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR}

$[BUILD_PTZENDNN]

FROM tfzendnn_installer_${ENABLE_TFZENDNN} AS ptzendnn_installer_no

FROM tfzendnn_installer_${ENABLE_TFZENDNN} AS ptzendnn_installer_yes

ARG COPY_DIR

COPY --from=ptzendnn_builder ${COPY_DIR} /

FROM ptzendnn_installer_${ENABLE_PTZENDNN} AS migraphx_installer_no

FROM ptzendnn_installer_${ENABLE_PTZENDNN} AS migraphx_installer_yes

ARG COPY_DIR

$[INSTALL_MIGRAPHX]

FROM common_builder AS builder_dev

ARG COPY_DIR
ARG AMDINFER_ROOT
ARG ENABLE_VITIS
SHELL ["/bin/bash", "-c"]

# delete any inherited artifacts and recreate
RUN rm -rf ${COPY_DIR} && mkdir ${COPY_DIR}

# pyinstaller 4.6 has a bug https://github.com/pyinstaller/pyinstaller/issues/6331
RUN pip3 install --no-cache-dir "pyinstaller!=4.6" \
    # docker-systemctl-replacement v1.5.4504
    && cd /tmp && wget --quiet https://github.com/gdraheim/docker-systemctl-replacement/archive/refs/tags/v1.5.4505.tar.gz \
    && tar -xzf v1.5.4505.tar.gz \
    && cd docker-systemctl-replacement-1.5.4505 \
    && pyinstaller files/docker/systemctl3.py --onefile \
    && chmod a+x dist/systemctl3 \
    && mkdir -p ${COPY_DIR}/usr/bin/  \
    && cp dist/systemctl3 ${COPY_DIR}/usr/bin/systemctl \
    && rm -fr /tmp/*

COPY . $AMDINFER_ROOT

RUN if [[ ${ENABLE_VITIS} == "yes" ]]; then \
        # make binary for custom script to get FPGAs
        pyinstaller $AMDINFER_ROOT/docker/fpga_util.py --onefile \
        && chmod a+x dist/fpga_util \
        && mkdir -p ${COPY_DIR}/usr/local/bin/ \
        && cp dist/fpga_util ${COPY_DIR}/usr/local/bin/fpga-util; \
    fi

FROM migraphx_installer_${ENABLE_MIGRAPHX} AS dev

ARG COPY_DIR
ARG AMDINFER_ROOT
ARG UNAME
ARG TARGETPLATFORM
SHELL ["/bin/bash", "-c"]

$[INSTALL_DEV_PACKAGES]

$[INSTALL_PYTHON_PACKAGES]

COPY --from=builder ${COPY_DIR} /
COPY --from=common_builder ${COPY_DIR} /
COPY --from=builder_dev ${COPY_DIR} /
COPY --from=builder_dev $AMDINFER_ROOT/docker/entrypoint.sh /root/entrypoint.sh
COPY --from=builder_dev $AMDINFER_ROOT/docker/.bash* /home/${UNAME}/
COPY --from=builder_dev $AMDINFER_ROOT/docker/.env /home/${UNAME}/

# run any final commands before finishing the dev image
RUN git lfs install \
    && npm install -g gh-pages \
    && ldconfig

$[ENTRYPOINT_DEV]

FROM ${DEV_BASE_IMAGE} AS builder_prod

ARG COPY_DIR
ARG MANIFESTS_DIR
ARG AMDINFER_ROOT
ARG ENABLE_VITIS
ARG ENABLE_MIGRAPHX

COPY . $AMDINFER_ROOT

RUN ldconfig \
    # delete any inherited artifacts and recreate
    && rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR} \
    # install libamdinfer.so
    && cd ${AMDINFER_ROOT} \
    && ./amdinfer install \
    && ./amdinfer install --get-manifest | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && ./amdinfer install --get-manifest > ${MANIFESTS_DIR}/amdinfer.txt \
    # build the static GUI files
    # && cd src/gui && npm install && npm run build \
    # get all the runtime shared library dependencies for the server
    && cd ${AMDINFER_ROOT} \
    && ./docker/get_dynamic_dependencies.sh --vitis ${ENABLE_VITIS} > ${MANIFESTS_DIR}/prod.txt \
    && ./docker/get_dynamic_dependencies.sh --copy ${COPY_DIR} --vitis ${ENABLE_VITIS}

FROM ${BASE_IMAGE} AS vitis_installer_prod_yes

ARG COPY_DIR
ARG AMDINFER_ROOT

# get AKS kernels
COPY --from=builder_prod $AMDINFER_ROOT/external/aks/libs/ /opt/xilinx/amdinfer/aks/libs/
# get the fpga-util executable
COPY --from=builder_prod /usr/local/bin/fpga-util /opt/xilinx/amdinfer/bin/

# we need the xclbins in the image and they must be copied from a path local
# to the build tree. But we also need this hack so the copy doesn't fail
# if this directory doesn't exist
COPY --from=builder_prod $AMDINFER_ROOT/docker/.env $AMDINFER_ROOT/external/overlaybin[s]/ /opt/xilinx/overlaybins/

# get the pre-defined AKS graphs and kernels
COPY --from=builder_prod $AMDINFER_ROOT/external/aks/graph_zoo/ /opt/xilinx/amdinfer/aks/graph_zoo/
COPY --from=builder_prod $AMDINFER_ROOT/external/aks/kernel_zoo/ /opt/xilinx/amdinfer/aks/kernel_zoo/

ENV LD_LIBRARY_PATH="/opt/xilinx/amdinfer/aks"
ENV XILINX_XRT="/opt/xilinx/xrt"
# TODO(varunsh): we shouldn't hardcode dpuv3int8 here
ENV XLNX_VART_FIRMWARE="/opt/xilinx/overlaybins/dpuv3int8"
ENV AKS_ROOT="/opt/xilinx/amdinfer/aks"
ENV AKS_XMODEL_ROOT="/opt/xilinx/amdinfer"
ENV PATH="/opt/xilinx/amdinfer/bin:${PATH}"

FROM ${BASE_IMAGE} AS vitis_installer_prod_no

FROM vitis_installer_prod_${ENABLE_VITIS} AS migraphx_installer_prod_no

FROM vitis_installer_prod_${ENABLE_VITIS} AS migraphx_installer_prod_yes

ARG COPY_DIR

$[INSTALL_MIGRAPHX]

FROM migraphx_installer_prod_${ENABLE_MIGRAPHX} as prod

ARG AMDINFER_ROOT
ARG COPY_DIR
ARG UNAME
WORKDIR /home/${UNAME}

# get all the installed files: the server, workers, C++ headers and dependencies
COPY --from=builder_prod ${COPY_DIR} /

# get the static gui files
# COPY --from=builder_prod $AMDINFER_ROOT/src/gui/build/ /opt/xilinx/amdinfer/gui/
# get the entrypoint script
COPY --from=builder_prod $AMDINFER_ROOT/docker/entrypoint.sh /root/entrypoint.sh
# get the systemctl executable - pulled in by get_dynamic_dependencies.sh
# COPY --from=builder_dev ${COPY_DIR}/bin/systemctl /bin/systemctl
# get the gosu executable
COPY --from=builder_prod /usr/local/bin/gosu /usr/local/bin/
# get the .bashrc and .env to configure the environment for all shells
COPY --from=builder_prod $AMDINFER_ROOT/docker/.bash* $AMDINFER_ROOT/docker/.env /home/${UNAME}/
COPY --from=builder_prod $AMDINFER_ROOT/docker/.root_bashrc /root/.bashrc
COPY --from=builder_prod $AMDINFER_ROOT/docker/.env /root/

# run any final commands before finishing the production image
RUN echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \
    && ldconfig

# we need to run as root because KServe mounts models to /mnt/models which means
# the server needs root access to access the mounted assets
ENTRYPOINT [ "/root/entrypoint.sh", "root" ]
CMD [ "amdinfer-server" ]

FROM ${IMAGE_TYPE} AS final

ARG ENABLE_VITIS
ARG ENABLE_TFZENDNN
ARG ENABLE_PTZENDNN
ARG ENABLE_MIGRAPHX

ARG UNAME
ARG GNAME
ARG AMDINFER_ROOT

ARG UID=1000
ARG GID=1000

ENV TZ=America/Los_Angeles
ENV LANG=en_US.UTF-8
ENV AMDINFER_ROOT=$AMDINFER_ROOT

$[SET_LOCALE]
    # set up timezone
    && ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone \
    # set up locale
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias $LANG

# add a user
$[ADD_USER]

LABEL project="amdinfer"
LABEL vitis=${ENABLE_VITIS}
LABEL tfzendnn=${ENABLE_TFZENDNN}
LABEL ptzendnn=${ENABLE_PTZENDNN}
LABEL migraphx=${ENABLE_MIGRAPHX}
