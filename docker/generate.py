import argparse
import pathlib
import sys
import textwrap


def set_locale_and_timezone(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    locales \\
                    sudo \\
                    tzdata \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/* \\"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            RUN yum -y update \\
                && yum -y install \\
                    sudo \\
                # clean up
                && yum clean all \\
                && rm -rf /var/cache/yum \\"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def add_user(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            RUN groupadd -g $GID -o $GNAME \\
                && useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME \\
                && passwd -d $UNAME \\
                && usermod -aG sudo $UNAME \\
                && echo 'ALL ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers \\
                && usermod -aG $GNAME root"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            RUN groupadd -g $GID -o $GNAME \\
                && useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME \\
                && passwd -d $UNAME \\
                && usermod -aG wheel $UNAME \\
                && echo 'ALL ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers \\
                && usermod -aG $GNAME root"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def add_dev_tools(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    ca-certificates \\
                    git \\
                    make \\
                    # used to get packages
                    wget \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/*"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            RUN yum -y update \\
                && yum -y install \\
                    ca-certificates \\
                    git \\
                    make \\
                    # used to get packages
                    wget \\
                # clean up
                && yum clean all \\
                && rm -rf /var/cache/yum"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def add_compiler(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    # add the add-apt-repository command
                    software-properties-common \\
                # install gcc-9 for a newer compiler
                && add-apt-repository -y ppa:ubuntu-toolchain-r/test \\
                && apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    gcc-9 \\
                    g++-9 \\
                # link gcc-9 and g++-9 to gcc and g++
                && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 \\
                    --slave /usr/bin/g++ g++ /usr/bin/g++-9 \\
                    --slave /usr/bin/gcov gcov /usr/bin/gcov-9 \\
                && apt-get -y purge --auto-remove software-properties-common \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/*"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            RUN yum -y update \\
                && yum -y install \\
                    centos-release-scl \\
                    devtoolset-9 \\
                && echo "scl enable devtoolset-9 bash"  >> /etc/bashrc \\
                # clean up
                && yum clean all \\
                && rm -rf /var/cache/yum"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def install_build_packages(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    # used by many
                    python3-dev \\
                    python3-pip \\
                    python3-setuptools \\
                    python3-wheel \\
                    # used by NodeJS
                    xz-utils \\
                    # used by Drogon
                    libbrotli-dev \\
                    libc-ares-dev \\
                    uuid-dev \\
                    # used by ffmpeg to build
                    nasm \\
                    # used by opencv, Vitis AI
                    pkg-config \\
                    # used by thrift to build
                    bison \\
                    flex \\
                    # used by opentelemetry
                    libcurl4-openssl-dev \\
                    # used by Vitis AI
                    libgoogle-glog-dev \\
                    # used by Drogon and Vitis AI
                    libssl-dev \\
                    # used by TF/PT ZenDNN building and protobuf
                    zip \\
                    unzip \\
                    # used by Drogon and pyinstaller
                    zlib1g-dev \\
                    # used by protobuf
                    autoconf \\
                    automake \\
                    curl \\
                    libtool \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/*"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            RUN yum -y update \\
                && yum -y install \\
                    # used by many
                    python3-devel \\
                    python3-pip \\
                    python3-setuptools \\
                    python3-wheel \\
                    # used by NodeJS
                    xz \\
                    # used by Drogon
                    brotli-devel \\
                    c-ares-devel \\
                    uuid-devel \\
                    # used by ffmpeg to build
                    nasm \\
                    # used by opencv, Vitis AI
                    pkgconfig \\
                    # used by thrift to build
                    bison \\
                    flex \\
                    # used by opentelemetry
                    libcurl-devel \\
                    # used by Vitis AI
                    glog-devel \\
                    # used by Drogon and Vitis AI
                    openssl-devel \\
                    # used by TF/PT ZenDNN building
                    zip \\
                    unzip \\
                    # used by Drogon and pyinstaller
                    zlib-devel \\
                # clean up
                && yum clean all \\
                && rm -rf /var/cache/yum"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def install_optional_build_packages(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    # used by include-what-you-use to build
                    libclang-10-dev \\
                    clang-10 \\
                    llvm-10-dev \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/*"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            # N/A"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def install_xrt(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            RUN apt-get update \\
                && if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \\
                    cd /tmp \\
                    && wget --quiet -O xrt.deb https://www.xilinx.com/bin/public/openDownload?filename=xrt_202120.2.12.427_18.04-amd64-xrt.deb \\
                    && wget --quiet -O xrm.deb https://www.xilinx.com/bin/public/openDownload?filename=xrm_202120.1.3.29_18.04-x86_64.deb \\
                    && apt-get update -y \\
                    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                        ./xrt.deb \\
                        ./xrm.deb \\
                    # copy over debs to COPY_DIR so we can install them as debs in
                    # the final image. In particular, the same XRT needs to also be
                    # installed on the host and so a .deb allows for easy version control
                    && cp ./xrt.deb ${COPY_DIR} \\
                    && cp ./xrm.deb ${COPY_DIR}; \\
                    # clean up
                    && apt-get clean -y \\
                    && rm -rf /var/lib/apt/lists/* \\
                fi;"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            RUN yum -y update \\
                && if [[ ${TARGETPLATFORM} == "linux/amd64" ]]; then \\
                    cd /tmp \\
                    && wget --quiet -O xrt.rpm https://www.xilinx.com/bin/public/openDownload?filename=xrt_202120.2.12.427_7.8.2003-x86_64-xrt.rpm \\
                    && wget --quiet -O xrm.rpm https://www.xilinx.com/bin/public/openDownload?filename=xrm_202120.1.3.29_7.8.2003-x86_64.rpm \\
                    && yum -y update \\
                    && yum -y install \\
                        ./xrt.rpm \\
                        ./xrm.rpm \\
                    # copy over rpms to COPY_DIR so we can install them as rpms in
                    # the final image. In particular, the same XRT needs to also be
                    # installed on the host and so a .rpm allows for easy version control
                    && cp ./xrt.rpm ${COPY_DIR} \\
                    && cp ./xrm.rpm ${COPY_DIR}; \\
                    # clean up
                    && yum clean all \\
                    && rm -rf /var/cache/yum \\
                fi;"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def build_optional():

    return textwrap.dedent(
        """\
        # install lcov 1.15 for test coverage measurement
        RUN wget --quiet https://github.com/linux-test-project/lcov/releases/download/v1.15/lcov-1.15.tar.gz \\
            && tar -xzf lcov-1.15.tar.gz \\
            && cd lcov-1.15 \\
            && checkinstall -y --pkgname lcov --pkgversion 1.15 --pkgrelease 1 make install \\
            && cd /tmp \\
            && dpkg -L lcov | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \\
            && dpkg -L lcov > ${MANIFESTS_DIR}/lcov.txt \\
            && rm -rf /tmp/*

        # install wrk for http benchmarking
        RUN wget --quiet https://github.com/wg/wrk/archive/refs/tags/4.1.0.tar.gz \\
            && tar -xzf 4.1.0.tar.gz \\
            && cd wrk-4.1.0 \\
            && make -j$(($(nproc) - 1)) \\
            && mkdir -p ${COPY_DIR}/usr/local/bin && cp wrk ${COPY_DIR}/usr/local/bin \\
            && rm -rf /tmp/*

        # install include-what-you-use 0.14
        RUN wget --quiet https://github.com/include-what-you-use/include-what-you-use/archive/refs/tags/0.14.tar.gz \\
            && tar -xzf 0.14.tar.gz \\
            && cd include-what-you-use-0.14 \\
            && mkdir build \\
            && cd build \\
            && cmake -DCMAKE_PREFIX_PATH=/usr/lib/llvm-10 .. \\
            && make -j$(($(nproc) - 1)) \\
            && make install \\
            && cat install_manifest.txt | xargs -i bash -c "if [ -f {} ]; then cp --parents -P {} ${COPY_DIR}; fi" \\
            && cat install_manifest.txt > ${MANIFESTS_DIR}/include_what_you_use.txt \\
            && cd /tmp \\
            && rm -fr /tmp/*
        """
    )


def install_vitis(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    # used by vitis ai runtime
                    libgoogle-glog-dev \\
                    # used to detect if ports are in use for XRM
                    net-tools \\
                    # used by libunilog as a fallback to find glog
                    pkg-config \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/*

            # install all the Vitis AI runtime libraries built from source as debs
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    /*.deb \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/* \\
                && rm /*.deb"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            RUN yum -y update \\
                && yum -y install \\
                    # used by vitis ai runtime
                    libgoogle-glog-dev \\
                    # used to detect if ports are in use for XRM
                    net-tools \\
                    # used by libunilog as a fallback to find glog
                    pkg-config \\
                # clean up
                && yum clean all \\
                && rm -rf /var/cache/yum

            # install all the Vitis AI runtime libraries built from source as rpms
            RUN yum -y update \\
                && yum -y install \\
                    /*.rpm \\
                # clean up
                && yum clean all \\
                && rm -rf /var/cache/yum \\
                && rm /*.rpm"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def install_dev_packages(image_type, core):
    if image_type == "apt":
        if not core:
            optional_packages = """
                    # used for auto-completing bash commands
                    bash-completion \\
                    # used for git
                    gpg \\
                    # install debugging tools
                    gdb \\
                    valgrind \\
                    vim \\
                    # used for code formatting and style
                    clang-format-10 \\
                    clang-tidy-10 \\"""

            optional_post = """
                    # symlink the versioned clang-*-10 executables to clang-*
                    && ln -s /usr/bin/clang-format-10 /usr/bin/clang-format \\
                    && ln -s /usr/bin/clang-tidy-10 /usr/bin/clang-tidy \\"""
        else:
            optional_packages = ""
            optional_post = ""
        command = textwrap.dedent(
            f"""\
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    curl \\
                    make \\
                    openssh-client \\
                    python3 \\
                    python3-dev \\
                    python3-pip \\
                    # install documentation dependencies
                    doxygen \\
                    graphviz \\
                    # used to turn absolute symlinks into relative ones
                    symlinks \\
                    # used for z compression in proteus
                    zlib1g-dev \\
                    # used by drogon. It needs the -dev versions to pass Cmake
                    libbrotli-dev \\
                    libc-ares-dev \\
                    libssl-dev \\
                    uuid-dev \\{optional_packages}{optional_post}
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/*"""
        )
    elif image_type == "yum":
        if not core:
            optional_packages = """
                    # used for auto-completing bash commands
                    bash-completion \\
                    # used for git
                    gpg \\
                    # install debugging tools
                    gdb \\
                    valgrind \\
                    vim \\
                    # used for code formatting and style
                    clang-format-10 \\
                    clang-tidy-10 \\"""

            optional_post = """
                    # symlink the versioned clang-*-10 executables to clang-*
                    && ln -s /usr/bin/clang-format-10 /usr/bin/clang-format \\
                    && ln -s /usr/bin/clang-tidy-10 /usr/bin/clang-tidy \\"""
        else:
            optional_packages = ""
            optional_post = ""
        command = textwrap.dedent(
            """\
            RUN yum -y update \\
                && yum -y install \\
                    curl \\
                    make \\
                    openssh-clients \\
                    python3 \\
                    python3-devel \\
                    python3-pip \\
                    # install documentation dependencies
                    doxygen \\
                    graphviz \\
                    # used to turn absolute symlinks into relative ones
                    symlinks \\
                    # used for z compression in proteus
                    zlib-devel \\
                    # used by drogon. It needs the -dev versions to pass Cmake
                    brotli-devel \\
                    c-ares-devel \\
                    openssl-devel \\
                    uuid-devel \\
                # clean up
                && yum clean all \\
                && rm -rf /var/cache/yum"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def build_migraphx(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            # Install rbuild
            RUN echo "deb [arch=amd64 trusted=yes] http://repo.radeon.com/rocm/apt/5.0/ ubuntu main" > /etc/apt/sources.list.d/rocm.list \\
                && apt-get update && \\
                DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    aria2 \\
                    half \\
                    libnuma-dev \\
                    libpython3.6-dev \\
                    miopen-hip-dev \\
                    rocblas-dev \\
                    rocm-cmake \\
                    rocm-dev \\
                && ln -s /opt/rocm-* /opt/rocm \\
                && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \\
                && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \\
                && ldconfig \\
                && pip3 install --no-cache-dir https://github.com/RadeonOpenCompute/rbuild/archive/f74d130aac0405c7e6bc759d331f913a7577bd54.tar.gz \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/*\

            # Install MIGraphX from source
            RUN mkdir -p /migraphx \\
                && cd /migraphx && git clone --branch develop https://github.com/ROCmSoftwarePlatform/AMDMIGraphX src \\
                && cd /migraphx/src  && git checkout cb18b0b5722373c49f5c257380af206e13344735 \\
                # disable building documentation and tests. Is there a better way?
                && sed -i 's/^add_subdirectory(doc)/#&/' CMakeLists.txt \\
                && sed -i 's/^add_subdirectory(test)/#&/' CMakeLists.txt \\
                && rbuild package -d /migraphx/deps -B /migraphx/build --define "BUILD_TESTING=OFF" \\
                && cp /migraphx/build/*.deb ${COPY_DIR}"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            # Install rbuild
            RUN echo "[ROCm]\\nname=ROCm\\nbaseurl=https://repo.radeon.com/rocm/yum/5.0/\\nenabled=1\\ngpgcheck=1\\ngpgkey=https://repo.radeon.com/rocm/rocm.gpg.key" > /etc/yum.repos.d/rocm.repo \\
                && yum -y update && \\
                yum -y install \\
                    aria2 \\
                    half \\
                    libnuma-dev \\
                    libpython3.6-dev \\
                    miopen-hip-dev \\
                    rocblas-dev \\
                    rocm-cmake \\
                    rocm-dev \\
                && ln -s /opt/rocm-* /opt/rocm \\
                && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \\
                && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \\
                && ldconfig \\
                && pip3 install --no-cache-dir https://github.com/RadeonOpenCompute/rbuild/archive/f74d130aac0405c7e6bc759d331f913a7577bd54.tar.gz \\
                # clean up
                && yum clean all \\
                && rm -rf /var/cache/yum

            # Install MIGraphX from source
            RUN mkdir -p /migraphx \\
                && cd /migraphx && git clone --branch develop https://github.com/ROCmSoftwarePlatform/AMDMIGraphX src \\
                && cd /migraphx/src  && git checkout cb18b0b5722373c49f5c257380af206e13344735 \\
                # disable building documentation and tests. Is there a better way?
                && sed -i 's/^add_subdirectory(doc)/#&/' CMakeLists.txt \\
                && sed -i 's/^add_subdirectory(test)/#&/' CMakeLists.txt \\
                && rbuild package -d /migraphx/deps -B /migraphx/build --define "BUILD_TESTING=OFF" \\
                && cp /migraphx/build/*.rpm ${COPY_DIR}"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def install_migraphx_dev(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            # install all .deb files from the builder
            RUN echo "deb [arch=amd64 trusted=yes] http://repo.radeon.com/rocm/apt/5.0/ ubuntu main" > /etc/apt/sources.list.d/rocm.list \\
                && apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    python3-pip \\
                    rsync \\
                    # these packages are required to build with migraphx but don't get installed
                    # as dependencies. They also create /opt/rocm-* so we can make a symlink
                    # before installing migraphx
                    miopen-hip-dev \\
                    rocm-device-libs \\
                    rocblas-dev \\
                    libnuma1 \\
                && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \\
                && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \\
                && ldconfig \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    /*.deb \\
                # having symlinks between rocm-* and rocm complicates building the production
                # image so make /opt/rocm a real directory and move files over to it from
                # /opt/rocm-* but add a symlink for /opt/rocm-* for compatibility
                && dir=$(find /opt/ -maxdepth 1 -type d -name "rocm-*") \\
                && rm -rf /opt/rocm \\
                && mkdir /opt/rocm \\
                && rsync -a $dir/* /opt/rocm/ \\
                && rm -rf $dir \\
                && ln -s /opt/rocm $dir \\
                # install .whl from the builder
                && pip3 install --no-cache-dir /*.whl \\
                && rm -f /*.whl \\
                # clean up
                && apt-get purge -y --auto-remove python3-pip rsync \\
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/* \\
                && rm -f /*.deb"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            # install all .rpm files from the builder
            RUN echo "[ROCm]\\nname=ROCm\\nbaseurl=https://repo.radeon.com/rocm/yum/5.0/\\nenabled=1\\ngpgcheck=1\\ngpgkey=https://repo.radeon.com/rocm/rocm.gpg.key" > /etc/yum.repos.d/rocm.repo \\
                && yum -y update && \\
                yum -y install \\
                    python3-pip \\
                    rsync \\
                    # these packages are required to build with migraphx but don't get installed
                    # as dependencies. They also create /opt/rocm-* so we can make a symlink
                    # before installing migraphx
                    miopen-hip-dev \\
                    rocm-device-libs \\
                    rocblas-dev \\
                    libnuma1 \\
                && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \\
                && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \\
                && ldconfig \\
                && yum -y install \\
                    /*.rpm \\
                # having symlinks between rocm-* and rocm complicates building the production
                # image so make /opt/rocm a real directory and move files over to it from
                # /opt/rocm-* but add a symlink for /opt/rocm-* for compatibility
                && dir=$(find /opt/ -maxdepth 1 -type d -name "rocm-*") \\
                && rm -rf /opt/rocm \\
                && mkdir /opt/rocm \\
                && rsync -a $dir/* /opt/rocm/ \\
                && rm -rf $dir \\
                && ln -s /opt/rocm $dir \\
                # install .whl from the builder
                && pip3 install --no-cache-dir /*.whl \\
                && rm -f /*.whl \\
                # clean up
                && yum remove python3-pip rsync \\
                && yum clean all \\
                && rm -rf /var/cache/yum \\
                && rm -f /*.rpm"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def install_migraphx_prod(image_type):
    if image_type == "apt":
        command = textwrap.dedent(
            """\
            # install all .deb files from the builder
            RUN echo "deb [arch=amd64 trusted=yes] http://repo.radeon.com/rocm/apt/5.0/ ubuntu main" > /etc/apt/sources.list.d/rocm.list \\
                && apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    rsync \\
                    # these packages are required to build with migraphx but don't get installed
                    # as dependencies. They also create /opt/rocm-* so we can make a symlink
                    # before installing migraphx
                    miopen-hip-dev \\
                    rocm-device-libs \\
                    rocblas-dev \\
                    libnuma1 \\
                && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \\
                && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \\
                && ldconfig \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    /*.deb \\
                # having symlinks between rocm-* and rocm complicates building the production
                # image so move files over to rocm/ but add a symlink for compatibility
                && dir=$(find /opt/ -maxdepth 1 -type d -name "rocm-*") \\
                && rm -rf /opt/rocm \\
                && mkdir /opt/rocm \\
                && rsync -a $dir/* /opt/rocm/ \\
                && rm -rf $dir \\
                && ln -s /opt/rocm $dir \\
                # clean up
                && apt-get purge -y --auto-remove rsync \\
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/* \\
                && rm -f /*.deb"""
        )
    elif image_type == "yum":
        command = textwrap.dedent(
            """\
            # install all .deb files from the builder
            RUN echo "[ROCm]\\nname=ROCm\\nbaseurl=https://repo.radeon.com/rocm/yum/5.0/\\nenabled=1\\ngpgcheck=1\\ngpgkey=https://repo.radeon.com/rocm/rocm.gpg.key" > /etc/yum.repos.d/rocm.repo \\
                && yum -y update && \\
                yum -y install \\
                    rsync \\
                    # these packages are required to build with migraphx but don't get installed
                    # as dependencies. They also create /opt/rocm-* so we can make a symlink
                    # before installing migraphx
                    miopen-hip-dev \\
                    rocm-device-libs \\
                    rocblas-dev \\
                    libnuma1 \\
                && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \\
                && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \\
                && ldconfig \\
                && yum -y install \\
                    /*.rpm \\
                # having symlinks between rocm-* and rocm complicates building the production
                # image so move files over to rocm/ but add a symlink for compatibility
                && dir=$(find /opt/ -maxdepth 1 -type d -name "rocm-*") \\
                && rm -rf /opt/rocm \\
                && mkdir /opt/rocm \\
                && rsync -a $dir/* /opt/rocm/ \\
                && rm -rf $dir \\
                && ln -s /opt/rocm $dir \\
                # clean up
                && yum remove rsync \\
                && yum clean all \\
                && rm -rf /var/cache/yum\
                && rm -f /*.rpm"""
        )
    else:
        raise ValueError(f"Unknown base image type: {image_type}")

    return command


def generate(args: argparse.Namespace):
    template = pathlib.Path(__file__).parent.resolve() / "template.dockerfile"
    with open(template, "r") as f:
        dockerfile = f.read()

    dockerfile = dockerfile.replace("$[BASE_IMAGE]", args.base_image, 1)

    image_type = args.base_image_type
    dockerfile = dockerfile.replace(
        "$[SET_LOCALE]", set_locale_and_timezone(image_type)
    )

    dockerfile = dockerfile.replace("$[ADD_USER]", add_user(image_type))

    dockerfile = dockerfile.replace("$[ADD_DEV_TOOLS]", add_dev_tools(image_type))

    if args.skip_compiler:
        dockerfile = dockerfile.replace("$[ADD_COMPILER]", "# skipping a new compiler")
    else:
        dockerfile = dockerfile.replace("$[ADD_COMPILER]", add_compiler(image_type))

    dockerfile = dockerfile.replace("$[ADD_DEV_TOOLS]", add_dev_tools(image_type))

    dockerfile = dockerfile.replace(
        "$[INSTALL_BUILD_PACKAGES]", install_build_packages(image_type)
    )

    dockerfile = dockerfile.replace(
        "$[INSTALL_OPTIONAL_BUILD_PACKAGES]",
        install_optional_build_packages(image_type),
    )

    if args.core:
        dockerfile = dockerfile.replace("$[BUILD_OPTIONAL]", "# skipping optional")
    else:
        dockerfile = dockerfile.replace("$[BUILD_OPTIONAL]", build_optional())

    dockerfile = dockerfile.replace("$[INSTALL_XRT]", install_xrt(image_type))

    dockerfile = dockerfile.replace("$[INSTALL_VITIS]", install_vitis(image_type))

    dockerfile = dockerfile.replace(
        "$[INSTALL_DEV_PACKAGES]", install_dev_packages(image_type, args.core)
    )

    dockerfile = dockerfile.replace("$[BUILD_MIGRAPHX]", build_migraphx(image_type))

    dockerfile = dockerfile.replace(
        "$[INSTALL_MIGRAPHX_DEV]", install_migraphx_dev(image_type)
    )

    dockerfile = dockerfile.replace(
        "$[INSTALL_MIGRAPHX_PROD]", install_migraphx_prod(image_type)
    )

    with open(args.output_name, "w+") as f:
        f.write(dockerfile)


def get_parser():
    parser = argparse.ArgumentParser(
        prog="dockerize",
        description="Dockerfile builder",
        add_help=False,
    )

    command_group = parser.add_argument_group("Options")
    command_group.add_argument(
        "--base-image",
        action="store",
        default="ubuntu:18.04",
        help="base image to use",
    )
    command_group.add_argument(
        "--base-image-type",
        action="store",
        default="apt",
        help="type of the base image: Debian/Ubuntu (apt) or RH/CentOS (yum)",
    )
    command_group.add_argument(
        "--core",
        action="store_true",
        help="only install core packages required for compilation",
    )
    command_group.add_argument(
        "--skip-compiler",
        action="store_true",
        help="don't install a new compiler",
    )
    command_group.add_argument(
        "--output-name",
        action="store",
        help="name of the dockerfile to create",
        default="Dockerfile",
    )

    command_group.add_argument(
        "-h", "--help", action="help", help="show this help message and exit"
    )

    return parser


def main():
    parser = get_parser()
    args = parser.parse_args()

    if args.base_image_type == "yum" and not args.core:
        print("For base images with yum, --core must be specified")
        sys.exit(1)

    generate(args)


if __name__ == "__main__":
    main()
