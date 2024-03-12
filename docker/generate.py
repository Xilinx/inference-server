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

import abc
import argparse
import importlib.machinery
import importlib.util
import pathlib
import sys
import textwrap


class PackageManager(abc.ABC):
    @property
    @abc.abstractmethod
    def name(self):
        pass

    @property
    @abc.abstractmethod
    def update(self):
        pass

    @property
    @abc.abstractmethod
    def install(self):
        pass

    @property
    @abc.abstractmethod
    def remove(self):
        pass

    @property
    @abc.abstractmethod
    def clean(self):
        pass

    @property
    @abc.abstractmethod
    def package(self):
        pass


class Apt(PackageManager):
    name = "apt"
    update = "apt-get -y update"
    install = (
        "DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends"
    )
    remove = "apt-get purge -y --auto-remove"
    clean = textwrap.dedent(
        """\
        && apt-get clean -y \\
        && rm -rf /var/lib/apt/lists/*"""
    )
    package = "deb"


class Yum(PackageManager):
    name = "yum"
    update = "yum -y update"
    install = "yum -y install"
    remove = "yum remove"
    clean = textwrap.dedent(
        """\
        && yum clean all \\
        && rm -rf /var/cache/yum"""
    )
    package = "rpm"


def code_indent(text: str, indent_length: int, indent_char=" "):
    """
    Indent the text to the specified length

    Args:
        text (str): Text to indent
        indent_length (int): Number of characters to indent
        indent_char (str, optional): Character to use for indent. Defaults to ' '.
    """

    def indented_lines():
        for i, line in enumerate(text.splitlines(True)):
            if i:
                yield indent_char * indent_length + line if line.strip() else line
            else:
                yield line

    return "".join(indented_lines())


def set_locale_and_timezone(manager: PackageManager):
    if manager.name == "apt":
        packages = textwrap.dedent(
            """\
            locales \\
            sudo \\
            tzdata \\"""
        )
    elif manager.name == "yum":
        packages = textwrap.dedent(
            """\
            sudo \\"""
        )
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return textwrap.dedent(
        f"""\
        RUN {manager.update} \\
            && {manager.install} \\
                {code_indent(packages, 16)}
            # clean up
            {code_indent(manager.clean, 12)} \\"""
    )


def add_user(manager):
    if manager.name == "apt":
        group = "sudo"
    elif manager.name == "yum":
        group = "wheel"
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return textwrap.dedent(
        f"""\
        RUN groupadd -g $GID -o $GNAME \\
            && useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME \\
            && passwd -d $UNAME \\
            && usermod -aG {group} $UNAME \\
            && echo 'ALL ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers \\
            && usermod -aG $GNAME root"""
    )


def add_dev_tools(manager: PackageManager):
    if manager.name == "apt":
        cpp_package = "g++"
    elif manager.name == "yum":
        cpp_package = "gcc-c++"
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return textwrap.dedent(
        f"""\
        RUN {manager.update} \\
            && {manager.install} \\
                ca-certificates \\
                git \\
                # need cc for libb64, and gcc gets installed by xrt as a dependency
                gcc \\
                {cpp_package} \\
                make \\
                # used to get packages
                wget \\
            # clean up
            {code_indent(manager.clean, 12)}"""
    )


def add_compiler(manager: PackageManager):
    if manager.name == "apt":
        packages = textwrap.dedent(
            """\
            gcc-9 \\
            g++-9 \\"""
        )
    elif manager.name == "yum":
        packages = textwrap.dedent(
            """\
                centos-release-scl \\
                devtoolset-9 \\
            && echo "scl enable devtoolset-9 bash"  >> /etc/bashrc \\"""
        )
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return textwrap.dedent(
        f"""\
        RUN {manager.update} \\
            && {manager.install} \\
                {code_indent(packages, 16)}
            # clean up
            {code_indent(manager.clean, 12)}"""
    )


def install_build_packages(manager):
    if manager.name == "apt":
        packages = textwrap.dedent(
            """\
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
            # used by opentelemetry and XRT
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
            libtool \\"""
        )
        site_packages = '&& echo "/usr/lib/python3.8/site-packages" >> /usr/local/lib/python3.8/dist-packages/site-packages.pth \\'
    elif manager.name == "yum":
        packages = textwrap.dedent(
            """\
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
            gflags-devel \\
            # used by Drogon and Vitis AI
            openssl-devel \\
            # used by TF/PT ZenDNN building
            zip \\
            unzip \\
            # used by Drogon and pyinstaller
            zlib-devel \\"""
        )
        site_packages = "# no site-packages modifications needed"
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return textwrap.dedent(
        f"""\
            RUN {manager.update} \\
                && {manager.install} \\
                    {code_indent(packages, 20)}
                {code_indent(site_packages, 16)}
                # clean up
                {code_indent(manager.clean, 16)}"""
    )


def install_optional_build_packages(manager: PackageManager):
    if manager.name == "apt":
        command = textwrap.dedent(
            """\
            RUN apt-get update \\
                && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \\
                    # used by include-what-you-use and pybind11_mkdoc
                    libclang-10-dev \\
                    clang-10 \\
                    llvm-10-dev \\
                # clean up
                && apt-get clean -y \\
                && rm -rf /var/lib/apt/lists/*"""
        )
    elif manager.name == "yum":
        command = textwrap.dedent(
            """\
            # N/A"""
        )
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return command


def get_xrm_xrt_packages(package_manager):
    if package_manager == "apt":
        return textwrap.dedent(
            """\
            && wget --quiet -O xrt.deb https://www.xilinx.com/bin/public/openDownload?filename=xrt_202220.2.14.418_20.04-amd64-xrt.deb \\
            && wget --quiet -O xrm.deb https://www.xilinx.com/bin/public/openDownload?filename=xrm_202220.1.5.212_20.04-x86_64.deb \\"""
        )
    elif package_manager == "yum":
        return textwrap.dedent(
            """\
            && wget --quiet -O xrt.rpm https://www.xilinx.com/bin/public/openDownload?filename=xrt_202220.2.14.418_7.8.2003-x86_64-xrt.rpm \\
            && wget --quiet -O xrm.rpm https://www.xilinx.com/bin/public/openDownload?filename=xrm_202220.1.5.212_7.8.2003-x86_64.rpm \\"""
        )
    raise ValueError(f"Unknown base image type: {package_manager}")


def install_xrt(manager: PackageManager, custom_backends):

    packages = (
        get_xrm_xrt_packages(manager.name)
        if custom_backends is None
        else custom_backends.get_xrm_xrt_packages(manager.name)
    )

    return textwrap.dedent(
        f"""\
        RUN {manager.update} \\
            && if [[ ${{TARGETPLATFORM}} == "linux/amd64" ]]; then \\
                cd /tmp \\
                {code_indent(packages, 16)}
                && {manager.install} \\
                    ./xrt.{manager.package} \\
                    ./xrm.{manager.package} \\
                # copy over {manager.package}s to COPY_DIR so we can install them as {manager.package}s in
                # the final image. In particular, the same XRT needs to also be
                # installed on the host and so a .{manager.package} allows for easy version control
                && cp ./xrt.{manager.package} ${{COPY_DIR}} \\
                && cp ./xrm.{manager.package} ${{COPY_DIR}} \\
                # clean up
                {code_indent(manager.clean, 16)}; \\
            fi;"""
    )


def build_optional():

    return textwrap.dedent(
        """\
        # install lcov 1.15 for test coverage measurement
        RUN wget --quiet https://github.com/linux-test-project/lcov/releases/download/v1.15/lcov-1.15.tar.gz \\
            && tar -xzf lcov-1.15.tar.gz \\
            && cd lcov-1.15 \\
            && INSTALL_DIR=/tmp/installed \\
            && mkdir -p ${INSTALL_DIR} \\
            && make install DESTDIR=${INSTALL_DIR} \\
            && find ${INSTALL_DIR} -type f | sed 's/\/tmp\/installed//' > ${MANIFESTS_DIR}/lcov.txt \\
            && cp -rP ${INSTALL_DIR}/* / \\
            && cat ${MANIFESTS_DIR}/lcov.txt | xargs -i bash -c "cp --parents -P {} ${COPY_DIR}" \\
            && cd /tmp \\
            && rm -rf /tmp/*

        # install wrk for http benchmarking
        RUN wget --quiet https://github.com/wg/wrk/archive/refs/tags/4.2.0.tar.gz \\
            && tar -xzf 4.2.0.tar.gz \\
            && cd wrk-4.2.0 \\
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


def install_vitis(manager: PackageManager):
    if manager.name == "apt":
        packages = textwrap.dedent(
            """\
            # used by vitis ai runtime
                libgoogle-glog-dev \\"""
        )
    elif manager.name == "yum":
        packages = textwrap.dedent(
            """\
            # used by vitis ai runtime
                glog-devel \\"""
        )
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return textwrap.dedent(
        f"""\
        RUN {manager.update} \\
            && {manager.install} \\
                {code_indent(packages, 16)}
                # used to detect if ports are in use for XRM
                net-tools \\
                # used by libunilog as a fallback to find glog
                pkg-config \\
            # clean up
            {code_indent(manager.clean, 12)}

        # install all the Vitis AI runtime libraries built from source as {manager.package}s
        RUN {manager.update} \\
            && {manager.install} \\
                /*.{manager.package} \\
            # clean up
            {code_indent(manager.clean, 12)} \\
            && rm /*.{manager.package}"""
    )


def build_tfzendnn():
    return textwrap.dedent(
        """\
        ARG TFZENDNN_PATH
        COPY $TFZENDNN_PATH /tmp/

        RUN unzip -q $(basename $TFZENDNN_PATH) \\
            && cd $(basename ${TFZENDNN_PATH%.*}) \\
            # To avoid protobuf version issues, create subfolder and copy include files
            && mkdir -p ${COPY_DIR}/usr/include/tfzendnn/ \\
            && mkdir -p ${COPY_DIR}/usr/lib \\
            # copy and list files that are copied
            && cp -rv include/* ${COPY_DIR}/usr/include/tfzendnn | cut -d"'" -f 2 | sed 's/include/\/usr\/include\/tfzendnn/' > ${MANIFESTS_DIR}/tfzendnn.txt \\
            && cp -rv lib/*.so* ${COPY_DIR}/usr/lib | cut -d"'" -f 2 | sed 's/lib/\/usr\/lib/' >> ${MANIFESTS_DIR}/tfzendnn.txt"""
    )


def build_ptzendnn():
    return textwrap.dedent(
        """\
        ARG PTZENDNN_PATH
        COPY $PTZENDNN_PATH /tmp/

        RUN unzip -q $(basename $PTZENDNN_PATH) \\
            && cd $(basename ${PTZENDNN_PATH%.*}) \\
            # To avoid protobuf version issues, create subfolder and copy include files
            && mkdir -p ${COPY_DIR}/usr/include/ptzendnn/ \\
            && mkdir -p ${COPY_DIR}/usr/lib \\
            # copy and list files that are copied
            && cp -rv include/* ${COPY_DIR}/usr/include/ptzendnn | cut -d"'" -f 2 | sed 's/include/\/usr\/include\/ptzendnn/' > ${MANIFESTS_DIR}/ptzendnn.txt \\
            && cp -rv lib/*.so* ${COPY_DIR}/usr/lib | cut -d"'" -f 2 | sed 's/lib/\/usr\/lib/' >> ${MANIFESTS_DIR}/ptzendnn.txt"""
    )

def install_dev_packages(manager: PackageManager, core):
    if manager.name == "apt":
        if not core:
            optional_packages = textwrap.dedent(
                """\
                # used for auto-completing bash commands
                    bash-completion \\
                    # used for git
                    gnupg2 \\
                    # install debugging tools
                    gdb \\
                    valgrind \\
                    vim \\
                    # used for code formatting and style
                    clang-format-10 \\
                    clang-tidy-10 \\
                    # for vcpkg
                    unzip \\
                    zip \\
                    pkg-config \\
                    nasm \\
                # symlink the versioned clang-*-10 executables to clang-*
                && ln -s /usr/bin/clang-format-10 /usr/bin/clang-format \\
                && ln -s /usr/bin/clang-tidy-10 /usr/bin/clang-tidy \\"""
            )
        else:
            optional_packages = "# skipping optional packages"
        packages = textwrap.dedent(
            f"""\
            curl \\
            file \\
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
            # used for z compression
            zlib1g-dev \\
            # used by drogon. It needs the -dev versions to pass Cmake
            libbrotli-dev \\
            libssl-dev \\
            uuid-dev \\
            {code_indent(optional_packages, 8)}"""
        )
    elif manager.name == "yum":
        if not core:
            optional_packages = textwrap.dedent(
                """\
                # used for auto-completing bash commands
                    bash-completion \\
                    # used for git
                    gnupg \\
                    # install debugging tools
                    gdb \\
                    valgrind \\
                    vim \\
                    # used for code formatting and style
                    clang-format-10 \\
                    clang-tidy-10 \\
                # symlink the versioned clang-*-10 executables to clang-*
                && ln -s /usr/bin/clang-format-10 /usr/bin/clang-format \\
                && ln -s /usr/bin/clang-tidy-10 /usr/bin/clang-tidy \\"""
            )
        else:
            optional_packages = "# skipping optional packages"
        packages = textwrap.dedent(
            f"""\
            curl \\
            file \\
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
            # used for z compression
            zlib-devel \\
            # used by drogon. It needs the -dev versions to pass Cmake
            brotli-devel \\
            openssl-devel \\
            uuid-devel \\
            {code_indent(optional_packages, 12)}"""
        )
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return textwrap.dedent(
        f"""\
        RUN {manager.update} \\
            && {manager.install} \\
            {code_indent(packages, 12)}
            # clean up
            {code_indent(manager.clean, 12)}"""
    )


def install_migraphx(manager: PackageManager, custom_backends):
    migraphx_apt_repo = 'echo "deb [arch=amd64 trusted=yes] http://repo.radeon.com/rocm/apt/5.6.1/ ubuntu main" > /etc/apt/sources.list.d/rocm.list'
    migraphx_yum_repo = '"[ROCm]\\nname=ROCm\\nbaseurl=https://repo.radeon.com/rocm/yum/5.6.1/\\nenabled=1\\ngpgcheck=1\\ngpgkey=https://repo.radeon.com/rocm/rocm.gpg.key" > /etc/yum.repos.d/rocm.repo'

    if manager.name == "apt":
        add_repo = (
            migraphx_apt_repo
            if custom_backends is None
            else custom_backends.migraphx_apt_repo
        )
    elif manager.name == "yum":
        add_repo = (
            migraphx_yum_repo
            if custom_backends is None
            else custom_backends.migraphx_yum_repo
        )
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")

    return textwrap.dedent(
        f"""\
        # install all .{manager.package} files from the builder
        RUN {add_repo} \\
            && {manager.update} \\
            && {manager.install} \\
                rsync \\
                # these packages are required to build with migraphx but don't get installed
                # as dependencies. They also create /opt/rocm-* so we can make a symlink
                # before installing migraphx
                miopen-hip-dev \\
                rocm-device-libs \\
                # a runtime requirement for miopen that isn't automatically included
                roctracer \\
                rocblas-dev \\
                libnuma1 \\
                migraphx-dev \\
            && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \\
            && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \\
            && ldconfig \\
            # having symlinks between rocm-* and rocm complicates building the production
            # image so move files over to rocm/ but add a symlink for compatibility
            && dir=$(find /opt/ -maxdepth 1 -type d -name "rocm-*") \\
            && rm -rf /opt/rocm \\
            && mkdir /opt/rocm \\
            && rsync -a $dir/* /opt/rocm/ \\
            && rm -rf $dir \\
            && ln -s /opt/rocm $dir \\
            # clean up
            && {manager.remove} \\
                rsync \\
            {code_indent(manager.clean, 12)}"""
    )

def install_rocal(manager: PackageManager, custom_backends):
    if manager.name == "yum":
        raise ValueError(f"rocAL installation not supported with : {manager.name}")
    
    amdgpu_install_deb = "apt install -y ./amdgpu-install_6.0.60002-1_all.deb"
    # no-dkms flag for intalling rocm inside docker
    amdgpu_install_rocm = "DEBIAN_FRONTEND=noninteractive amdgpu-install -y --usecase=graphics,rocm --no-32 --no-dkms"

    return textwrap.dedent(
        f"""\
        RUN {manager.update} \\
        && wget --quiet https://repo.radeon.com/amdgpu-install/6.0.2/ubuntu/focal/amdgpu-install_6.0.60002-1_all.deb \\
        && {amdgpu_install_deb} \\
        && {amdgpu_install_rocm} \\
        && rm amdgpu-install_6.0.60002-1_all.deb \\
        && {manager.install} \\
            clang \\
            libomp-dev \\
            rsync \\
            rpp-dev \\
            libjpeg-dev \\
        && echo "/opt/rocm/lib" > /etc/ld.so.conf.d/rocm.conf \\
        && echo "/opt/rocm/llvm/lib" > /etc/ld.so.conf.d/rocm-llvm.conf \\
        && ldconfig \\
        # having symlinks between rocm-* and rocm complicates building the production
        # image so move files over to rocm/ but add a symlink for compatibility
        && dir=$(find /opt/ -maxdepth 1 -type d -name "rocm-*") \\
        && rm -rf /opt/rocm \\
        && mkdir /opt/rocm \\
        && rsync -a $dir/* /opt/rocm/ \\
        && rm -rf $dir \\
        && ln -s /opt/rocm $dir \\
        && {manager.remove} \\
            rsync \\
        {code_indent(manager.clean, 12)}
        """
    )

def install_python_packages():
    # The versions are pinned to prevent package updates from breaking the
    # container. To update this list, install the packages we actually want:
    #     pip install --no-cache-dir --ignore-installed \
    #         pytest \
    #         pytest-cpp \
    #         pytest-xprocess \
    #         requests \
    #         breathe \
    #         fastcov \
    #         sphinx \
    #         sphinx_copybutton \
    #         sphinxcontrib-confluencebuilder \
    #         sphinx-argparse \
    #         sphinxemoji \
    #         sphinx-issues \
    #         exhale \
    #         sphinx-favicon \
    #         sphinx-tabs \
    #         sphinx-tippy \
    #         sphinxcontrib-openapi \
    #         sphinxcontrib-jquery \
    #         sphinx-charts \
    #         black \
    #         cpplint \
    #         cmakelang  \
    #         pre-commit \
    #         opencv-python-headless \
    #         scikit-build \
    #         pytest-benchmark \
    #         rich \
    #         pybind11_mkdoc \
    #         pybind11-stubgen

    return textwrap.dedent(
        """\
        RUN python3 -m pip install --upgrade --force-reinstall pip \\
            && pip install --no-cache-dir \\
                # install these first
                setuptools \\
                wheel \\
            # clang-tidy-10 installs pyyaml which can't be uninstalled with pip
            && pip install --no-cache-dir --ignore-installed \\
                "alabaster==0.7.13" \\
                "attrs==23.1.0" \\
                "Babel==2.12.1" \\
                "beautifulsoup4==4.12.2" \\
                "black==23.3.0" \\
                "breathe==4.35.0" \\
                "certifi==2022.12.7" \\
                "cfgv==3.3.1" \\
                "charset-normalizer==3.1.0" \\
                "clang==14.0" \\
                "click==8.1.3" \\
                "cmakelang==0.6.13" \\
                "colorama==0.4.6" \\
                "cpplint==1.6.1" \\
                "deepmerge==1.1.0" \\
                "distlib==0.3.6" \\
                "distro==1.8.0" \\
                "docutils==0.17.1" \\
                "exceptiongroup==1.1.1" \\
                "exhale==0.3.6" \\
                "fastcov==1.14" \\
                "filelock==3.12.0" \\
                "identify==2.5.22" \\
                "idna==3.4" \\
                "imagesize==1.4.1" \\
                "importlib-metadata==6.5.0" \\
                "importlib-resources==5.12.0" \\
                "iniconfig==2.0.0" \\
                "Jinja2==3.0.3" \\
                "jsonschema==4.17.3" \\
                "lxml==4.9.2" \\
                "markdown-it-py==2.2.0" \\
                "MarkupSafe==2.1.2" \\
                "mdurl==0.1.2" \\
                "mistune==2.0.5" \\
                "mypy-extensions==1.0.0" \\
                "nodeenv==1.7.0" \\
                "numpy==1.24.2" \\
                "opencv-python-headless==4.7.0.72" \\
                "packaging==23.1" \\
                "pathspec==0.11.1" \\
                "picobox==3.0.0" \\
                "pkgutil_resolve_name==1.3.10" \\
                "platformdirs==3.2.0" \\
                "pluggy==1.0.0" \\
                "pre-commit==3.2.2" \\
                "psutil==5.9.5" \\
                "py==1.11.0" \\
                "py-cpuinfo==9.0.0" \\
                "pybind11-stubgen==0.13.0" \\
                "pybind11_mkdoc==2.6.2" \\
                "Pygments==2.15.1" \\
                "pyrsistent==0.19.3" \\
                "pytest==7.3.1" \\
                "pytest-benchmark==4.0.0" \\
                "pytest-cpp==2.3.0" \\
                "pytest-xprocess==0.22.2" \\
                "pytz==2023.3" \\
                "PyYAML==6.0" \\
                "requests==2.28.2" \\
                "rich==13.3.4" \\
                "scikit-build==0.17.2" \\
                "six==1.16.0" \\
                "snowballstemmer==2.2.0" \\
                "soupsieve==2.4.1" \\
                "Sphinx==4.5.0" \\
                "sphinx-argparse==0.4.0" \\
                "sphinx-charts==0.2.1" \\
                "sphinx-copybutton==0.5.2" \\
                "sphinx-favicon==1.0.1" \\
                "sphinx-issues==3.0.1" \\
                "sphinx-tabs==3.4.0" \\
                "sphinx_mdinclude==0.5.3" \\
                "sphinx_tippy==0.4.1" \\
                "sphinxcontrib-applehelp==1.0.4" \\
                "sphinxcontrib-confluencebuilder==2.0.0" \\
                "sphinxcontrib-devhelp==1.0.2" \\
                "sphinxcontrib-htmlhelp==2.0.1" \\
                "sphinxcontrib-httpdomain==1.8.1" \\
                "sphinxcontrib-jquery==4.1" \\
                "sphinxcontrib-jsmath==1.0.1" \\
                "sphinxcontrib-openapi==0.8.1" \\
                "sphinxcontrib-qthelp==1.0.3" \\
                "sphinxcontrib-serializinghtml==1.1.5" \\
                "sphinxemoji==0.2.0" \\
                "tomli==2.0.1" \\
                "typing_extensions==4.5.0" \\
                "urllib3==1.26.15" \\
                "virtualenv==20.22.0" \\
                "zipp==3.15.0" \\
            # some packages install to site-packages which needs to be added to the PATH
            && echo "/usr/lib/python3.8/site-packages" >> /usr/local/lib/python3.8/dist-packages/site-packages.pth
        """
    )


def vcpkg_build(manager):
    if manager.name == "apt":
        return textwrap.dedent(
            f"""\
            RUN {manager.update} \\
                && {manager.install} \\
                    curl \\
                    zip \\
                    unzip \\
                    tar \\
                    nasm \\
                    pkg-config \\
                    python3-dev \\
                # clean up
                {code_indent(manager.clean, 16)}"""
        )
    elif manager.name == "yum":
        return textwrap.dedent(
            f"""\
            RUN {manager.update} \\
                && {manager.install} \\
                    curl \\
                    zip \\
                    unzip \\
                    tar \\
                    nasm \\
                    pkgconfig \\
                    python3-devel \\
                    perl-IPC-Cmd \\
                # clean up
                {code_indent(manager.clean, 16)}"""
        )
    else:
        raise ValueError(f"Unknown base image type: {manager.name}")


def generate(args: argparse.Namespace):
    if args.custom_backends:
        # load a specific python file without consideration about modules/
        # packages
        loader = importlib.machinery.SourceFileLoader(
            "custom_backends", args.custom_backends
        )
        spec = importlib.util.spec_from_loader(loader.name, loader)
        custom_backends = importlib.util.module_from_spec(spec)
        loader.exec_module(custom_backends)

    template = pathlib.Path(__file__).parent.resolve() / "template.dockerfile"
    with open(template, "r") as f:
        dockerfile = f.read()

    dockerfile = dockerfile.replace("$[BASE_IMAGE]", args.base_image, 1)

    if args.base_image_type == "apt":
        manager = Apt
    elif args.base_image_type == "yum":
        manager = Yum
    else:
        raise ValueError(f"Unknown base image type: {args.base_image_type}")

    dockerfile = dockerfile.replace("$[SET_LOCALE]", set_locale_and_timezone(manager))

    dockerfile = dockerfile.replace("$[ADD_USER]", add_user(manager))

    dockerfile = dockerfile.replace("$[ADD_DEV_TOOLS]", add_dev_tools(manager))

    if args.skip_compiler:
        dockerfile = dockerfile.replace("$[ADD_COMPILER]", "# skipping a new compiler")
    else:
        dockerfile = dockerfile.replace("$[ADD_COMPILER]", add_compiler(manager))

    dockerfile = dockerfile.replace("$[ADD_DEV_TOOLS]", add_dev_tools(manager))

    dockerfile = dockerfile.replace(
        "$[INSTALL_BUILD_PACKAGES]", install_build_packages(manager)
    )

    dockerfile = dockerfile.replace(
        "$[INSTALL_OPTIONAL_BUILD_PACKAGES]",
        install_optional_build_packages(manager),
    )

    if args.core:
        dockerfile = dockerfile.replace("$[BUILD_OPTIONAL]", "# skipping optional")
    else:
        dockerfile = dockerfile.replace("$[BUILD_OPTIONAL]", build_optional())

    if args.custom_backends:
        dockerfile = dockerfile.replace(
            "$[INSTALL_XRT]", install_xrt(manager, custom_backends)
        )
    else:
        dockerfile = dockerfile.replace("$[INSTALL_XRT]", install_xrt(manager, None))

    dockerfile = dockerfile.replace("$[INSTALL_VITIS]", install_vitis(manager))

    if args.custom_backends:
        dockerfile = dockerfile.replace(
            "$[BUILD_TFZENDNN]", custom_backends.build_tfzendnn()
        )
    else:
        dockerfile = dockerfile.replace("$[BUILD_TFZENDNN]", build_tfzendnn())

    if args.custom_backends:
        dockerfile = dockerfile.replace(
            "$[BUILD_PTZENDNN]", custom_backends.build_ptzendnn()
        )
    else:
        dockerfile = dockerfile.replace("$[BUILD_PTZENDNN]", build_ptzendnn())

    if args.custom_backends:
        dockerfile = dockerfile.replace(
            "$[INSTALL_ROCAL]", install_rocal(manager, custom_backends)
        )
    else:
        dockerfile = dockerfile.replace(
            "$[INSTALL_ROCAL]", install_rocal(manager, None)
        )

    dockerfile = dockerfile.replace(
        "$[INSTALL_DEV_PACKAGES]", install_dev_packages(manager, args.core)
    )

    if args.cibuildwheel:
        dockerfile = dockerfile.replace(
            "$[INSTALL_PYTHON_PACKAGES]", "# skipping python packages"
        )
    else:
        dockerfile = dockerfile.replace(
            "$[INSTALL_PYTHON_PACKAGES]", install_python_packages()
        )

    if args.custom_backends:
        dockerfile = dockerfile.replace(
            "$[INSTALL_MIGRAPHX]", install_migraphx(manager, custom_backends)
        )
    else:
        dockerfile = dockerfile.replace(
            "$[INSTALL_MIGRAPHX]", install_migraphx(manager, None)
        )

    dockerfile = dockerfile.replace("$[VCPKG_BUILD]", vcpkg_build(manager))

    if args.cibuildwheel:
        entrypoint = textwrap.dedent(
            """\
            # skipping entrypoint for cibuildwheel
            ENV AMDINFER_ROOT=/project"""
        )
    else:
        entrypoint = textwrap.dedent(
            """\
            ENTRYPOINT [ "/root/entrypoint.sh", "user"]
            CMD [ "/bin/bash" ]"""
        )
    dockerfile = dockerfile.replace("$[ENTRYPOINT_DEV]", entrypoint)

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
        default="ubuntu:20.04",
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
        "--cibuildwheel",
        action="store_true",
        help="build for cibuildwheel",
    )
    command_group.add_argument(
        "--custom-backends",
        action="store",
        default="",
        help="path to a custom script for building and installing backends",
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

    if args.cibuildwheel:
        args.base_image_type = "yum"
        args.core = True
        args.skip_compiler = True

    generate(args)


if __name__ == "__main__":
    main()
