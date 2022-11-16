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

usage() {
cat << EOF
usage: ./tools/update_aks.sh [direction] path/to/aks

This script force updates the AKS-related files. Note that the
files in this repo are similar to, but may slightly differ from, the AKS files
released in Vitis AI.

Direction should be "to" or "from"
EOF
}

copy_files() {
  for index in ${!srcs[*]}; do
    src=${srcs[$index]}
    dest=${dests[$index]}

    if [[ $1 == "copy" ]]; then
      rsync -arv --prune-empty-dirs --exclude="build/" --include "*/" --include="*.cpp" --include="*.hpp" --include="*.txt" --include="*.json" --exclude="*" "$src" "$dest"
    else
      rsync -arvn --prune-empty-dirs --exclude="build/" --include "*/" --include="*.cpp" --include="*.hpp" --include="*.txt" --include="*.json" --exclude="*" "$src" "$dest"
    fi
  done
}

if [[ "$#" != 2 ]]; then
  usage
  exit 1
fi

direction="$1"
aks_path="$2"
amdinfer_path="./external/aks/reference"

if [[ $direction != "to" && $direction != "from" ]]; then
  usage
  exit 1
fi

if [[ $direction == "to" ]]; then
  aks_src_path=$amdinfer_path
  aks_dst_path=$aks_path
else
  aks_src_path=$aks_path
  aks_dst_path=$amdinfer_path
fi

srcs=(
  "$aks_src_path/graph_zoo/"
  "$aks_src_path/kernel_src/"
  "$aks_src_path/kernel_zoo/"
)

dests=(
  "$aks_dst_path/graph_zoo"
  "$aks_dst_path/kernel_src"
  "$aks_dst_path/kernel_zoo"
)

if ! copy_files "test"; then
  exit 1
fi

# https://stackoverflow.com/a/3232082
read -r -p "Are you sure? [y/N] " response
response=${response,,} # to lower
if [[ "$response" =~ ^(yes|y)$ ]]; then
  copy_files "copy"
  cp $aks_src_path/cmake-kernels.sh $aks_dst_path
  mkdir -p "$aks_dst_path/libs"
fi
