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

# enable building backends. By default, all backends are opt-in
ARG ENABLE_VITIS=${ENABLE_VITIS:-no}
ARG ENABLE_TFZENDNN=${ENABLE_TFZENDNN:-no}
ARG TFZENDNN_PATH
ARG ENABLE_PTZENDNN=${ENABLE_PTZENDNN:-no}
ARG PTZENDNN_PATH
ARG ENABLE_MIGRAPHX=${ENABLE_MIGRAPHX:-no}
ARG ENABLE_ROCAL=${ENABLE_ROCAL:-no}

ARG GIT_USER
ARG GIT_TOKEN

# this stage adds development tools such as compilers to the base image. It's
# used as an ancestor for all development-related stages
FROM ${BASE_IMAGE} AS dev_base

ARG TARGETPLATFORM
SHELL ["/bin/bash", "-c"]

# install common dev tools
$[ADD_DEV_TOOLS]

# add a newer compiler if required
$[ADD_COMPILER]

# install Cmake 3.24.2
RUN if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \
        archive="cmake-3.24.2-linux-x86_64.tar.gz"; \
    elif [[ ${TARGETPLATFORM} == "linux/arm64" ]]; then \
        archive="cmake-3.24.2-linux-aarch64.tar.gz"; \
    else false; fi; \
    url="https://github.com/Kitware/CMake/releases/download/v3.24.2/${archive}" \
    && cd /tmp/ \
    && wget --quiet ${url} \
    && tar --strip-components=1 -xzf ${archive} -C /usr/local \
    && rm -rf /tmp/*

# this stage builds any common dependencies between the inference server and
# any of the backends. Using common packages between the two ensures version
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

# install pybind11 2.9.1 - used by Vitis AI and inference server
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

# install NodeJS 14.16.0 for web gui development and gh-pages
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

# Install XRT and XRM
$[INSTALL_XRT]

FROM dev_base AS vitis_installer_no

FROM dev_base AS vitis_installer_yes

ARG COPY_DIR
ENV AMDINFER_ENABLE_VITIS=ON
ENV AMDINFER_ENABLE_AKS=ON

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
ENV AMDINFER_ENABLE_TFZENDNN=ON

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
ENV AMDINFER_ENABLE_PTZENDNN=ON

COPY --from=ptzendnn_builder ${COPY_DIR} /

FROM ptzendnn_installer_${ENABLE_PTZENDNN} AS migraphx_installer_no

FROM ptzendnn_installer_${ENABLE_PTZENDNN} AS migraphx_installer_yes

ARG COPY_DIR
ENV AMDINFER_ENABLE_MIGRAPHX=ON

$[INSTALL_MIGRAPHX]

FROM migraphx_installer_${ENABLE_MIGRAPHX} AS rocal_installer_no

FROM migraphx_installer_${ENABLE_MIGRAPHX} AS rocal_installer_yes

ARG COPY_DIR
ENV AMDINFER_ENABLE_ROCAL=ON

$[INSTALL_ROCAL]

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

COPY docker/fpga_util.py /tmp/

RUN if [[ ${ENABLE_VITIS} == "yes" ]]; then \
        # make binary for custom script to get FPGAs
        pyinstaller /tmp/fpga_util.py --onefile \
        && chmod a+x dist/fpga_util \
        && mkdir -p ${COPY_DIR}/usr/local/bin/ \
    && cp dist/fpga_util ${COPY_DIR}/usr/local/bin/fpga-util; \
    fi

FROM rocal_installer_${ENABLE_ROCAL} AS vcpkg_builder

ARG ENABLE_VITIS
ARG COPY_DIR
ARG GIT_USER
ARG GIT_TOKEN
WORKDIR /tmp

$[VCPKG_BUILD]

COPY vcpkg.json /tmp/vcpkg.json
COPY external/vcpkg /tmp/external/vcpkg
COPY docker/install_vcpkg.sh /tmp/docker/install_vcpkg.sh
# copy pybind11
COPY --from=common_builder ${COPY_DIR} /

RUN mkdir /opt/vcpkg \
    && cd /opt/vcpkg \
    && wget --quiet https://github.com/microsoft/vcpkg/archive/refs/tags/2023.04.15.tar.gz \
    && tar -xzf 2023.04.15.tar.gz \
    && mv vcpkg-2023.04.15 vcpkg \
    && cd vcpkg \
    && ./bootstrap-vcpkg.sh -disableMetrics \
    && rm /opt/vcpkg/2023.04.15.tar.gz \
    && cd /tmp \
    && GIT_USER="${GIT_USER}" GIT_TOKEN="${GIT_TOKEN}" ./docker/install_vcpkg.sh --vitis ${ENABLE_VITIS} --rocal ${ENABLE_ROCAL}
FROM migraphx_installer_${ENABLE_MIGRAPHX} AS dev
FROM rocal_installer_${ENABLE_ROCAL} AS dev

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
COPY --from=vcpkg_builder /opt/vcpkg /opt/vcpkg
COPY docker/entrypoint.sh /root/entrypoint.sh
COPY docker/.bash* /home/${UNAME}/
COPY docker/.env /home/${UNAME}/

# run any final commands before finishing the dev image
RUN git lfs install \
    && npm install -g gh-pages \
    && echo "/opt/vcpkg/x64-linux-dynamic/lib" > /etc/ld.so.conf.d/vcpkg.conf \
    && ldconfig

$[ENTRYPOINT_DEV]

FROM ${DEV_BASE_IMAGE} AS builder_prod

ARG COPY_DIR
ARG MANIFESTS_DIR
ARG AMDINFER_ROOT
ARG ENABLE_VITIS
ARG ENABLE_MIGRAPHX
ARG ENABLE_TFZENDNN
ARG ENABLE_ROCAL

COPY . $AMDINFER_ROOT

RUN export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/vcpkg/x64-linux-dynamic/lib \
    && ldconfig \
    # delete any inherited artifacts and recreate
    && rm -rf ${COPY_DIR} && mkdir ${COPY_DIR} && mkdir -p ${MANIFESTS_DIR} \
    # install libamdinfer.so
    && cd ${AMDINFER_ROOT} \
    && ./amdinfer install \
    && ./amdinfer install --get-manifest | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \
    && ./amdinfer install --get-manifest > ${MANIFESTS_DIR}/amdinfer.txt \
    # get all the runtime shared library dependencies for the server
    && cd ${AMDINFER_ROOT} \
    # --migraphx is not passed since it's installed from debians below
    && ./docker/get_dynamic_dependencies.sh --vitis ${ENABLE_VITIS} --tfzendnn ${ENABLE_TFZENDNN} > ${MANIFESTS_DIR}/prod.txt \
    && ./docker/get_dynamic_dependencies.sh --copy ${COPY_DIR} --vitis ${ENABLE_VITIS} --tfzendnn ${ENABLE_TFZENDNN}

FROM ${BASE_IMAGE} AS vitis_installer_prod_yes

ARG COPY_DIR
ARG AMDINFER_ROOT

# get AKS kernels
COPY --from=builder_prod $AMDINFER_ROOT/external/aks/libs/* /opt/xilinx/amdinfer/aks/libs/
# get the fpga-util executable
COPY --from=builder_prod /usr/local/bin/fpga-util /opt/xilinx/amdinfer/bin/

# we need the xclbins in the image and they must be copied from a path local
# to the build tree. But we also need this hack so the copy doesn't fail
# if this directory doesn't exist
COPY --from=builder_prod $AMDINFER_ROOT/docker/.env $AMDINFER_ROOT/external/overlaybin[s]/ /opt/xilinx/overlaybins/

# get the pre-defined AKS graphs and kernels
COPY --from=builder_prod $AMDINFER_ROOT/external/aks/graph_zoo/ /opt/xilinx/amdinfer/aks/graph_zoo/
COPY --from=builder_prod $AMDINFER_ROOT/external/aks/kernel_zoo/ /opt/xilinx/amdinfer/aks/kernel_zoo/

ENV LD_LIBRARY_PATH="/opt/xilinx/amdinfer/aks:/opt/vcpkg/x64-linux-dynamic/lib"
ENV XILINX_XRT="/opt/xilinx/xrt"
# TODO(varunsh): we shouldn't hardcode dpuv3int8 here
ENV XLNX_VART_FIRMWARE="/opt/xilinx/overlaybins/dpuv3int8"
ENV AKS_ROOT="/opt/xilinx/amdinfer/aks"
ENV AKS_XMODEL_ROOT="/opt/xilinx/amdinfer"
ENV PATH="/opt/xilinx/amdinfer/bin:${PATH}"

RUN echo "/opt/xilinx/xrt/lib" > /etc/ld.so.conf.d/xrt.conf

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
    && echo "/opt/vcpkg/x64-linux-dynamic/lib" > /etc/ld.so.conf.d/vcpkg.conf \
    && ldconfig

# we need to run as root because KServe mounts models to /mnt/models which means
# the server needs root access to access the mounted assets
ENTRYPOINT [ "/root/entrypoint.sh", "root" ]
CMD [ "amdinfer-server", "--repository-load-existing" ]

FROM ${IMAGE_TYPE} AS final

ARG ENABLE_VITIS
ARG ENABLE_TFZENDNN
ARG ENABLE_PTZENDNN
ARG ENABLE_MIGRAPHX
ARG ENABLE_ROCAL

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
LABEL rocal=${ENABLE_ROCAL}
