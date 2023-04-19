#!/usr/bin/env bash
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

set -e

VERSION=dev
echo "Pushing to ./$VERSION. Okay?"
read response
if [[ "$response" != "y" && "$response" != "Y" ]]; then
    echo "Exiting"
    exit
fi

cache_dir="/tmp/node_modules/gh-pages/.cache"
mkdir -p "$cache_dir"

config=($(<./build/config.txt))
directory=${config[0]}
sed -i -e "s/inference-server\/main/inference-server\/$VERSION/g" ./README.rst
sed -i -e "s/version = \"main\"/version = \"$VERSION\"/g" ./docs/conf.py

./amdinfer make doxygen
./amdinfer make sphinx
export CACHE_DIR="$cache_dir"
# -t adds dotfiles
gh-pages -t -d ./build/$directory/docs/sphinx/ -e ./$VERSION

rm -rf "/tmp/node_modules"
sed -i -e "s/version = \"$VERSION\"/version = \"main\"/g" ./docs/conf.py
sed -i -e "s/inference-server\/$VERSION/inference-server\/main/g" ./README.rst
