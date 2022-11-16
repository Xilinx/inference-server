#!/usr/bin/env bash
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

#* Before building wheels for Linux, you first need to prepare an image to build in.
#* Create a Dockerfile for the type of wheel you want to build. Update the base-image
#* as needed from an appropriate base image:
#* https://cibuildwheel.readthedocs.io/en/stable/options/#linux-image
#*
#* $ python docker/generate.py --cibuildwheel --base-image quay.io/pypa/manylinux2014_x86_64
#*
#* Build the image. Use the appropriate platform flags and suffix to name your image
#* $ ./amdinfer dockerize --suffix="-ci"
#*
#* Add this image to the environment. In this case, it was a manylinux x86_64 image
#* export CIBW_MANYLINUX_X86_64_IMAGE=$(whoami)/amdinfer-dev-ci:latest


set -eo pipefail

if ! command -v cibuildwheel &> /dev/null
then
    echo "cibuildwheel not found in the PATH. Is it installed?"
    exit
fi

if [ -z "${CIBW_MANYLINUX_X86_64_IMAGE}" ]; then
    echo "CIBW_MANYLINUX_X86_64_IMAGE must be set in the environment:"
    echo "  the image to use to build the Python wheels for this platform"
    exit
fi

echo "This script should be run in a fresh clone of the repository. Continue?"
read response
if [[ "$response" != "y" && "$response" != "Y" ]]; then
    echo "Enter y or Y to confirm. Exiting"
    exit
fi

cibuildwheel --platform linux

echo "Built wheels are in ./wheelhouse/*"
