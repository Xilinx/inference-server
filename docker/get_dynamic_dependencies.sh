#!/usr/bin/env bash
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


usage() {
cat << EOF

EOF
}

ALL_ARGS=("$@")
# get the current directory
# DIR="$( cd "$( dirname "$0" )" && pwd )"
DEPS_FILE=deps.txt
UNIQUE_DEPS_FILE=deps_unique.txt

resolve_symlinks() {
  dep=$1

  dir=$(dirname $dep)
  while [[ -L $dep ]]; do
    # echo $dep
    echo "$dep" >> $DEPS_FILE
    dep=$(readlink $dep)
    # if the next path isn't absolute, append the directory from above and resolve full path
    if [[ ! "$dep" = /* ]]; then
      dep=$(realpath -s $dir/$dep)
    fi
  done
  echo "$dep" >> $DEPS_FILE
}

get_dependencies() {
  dynamic_file=$1

  # extract just the paths to the shared libraries
  readarray -t deps < <(ldd $dynamic_file | tr -s '[:blank:]' '\n' | grep '^/')

  for dep in "${deps[@]}"; do
    resolve_symlinks $dep
  done
}

add_vitis_deps() {
  # some vitis libraries are found in .xmodels or are otherwise not linked to.
  # As such, we don't find them in get_dependencies. We add them here manually

  vitis_debs=(
    xrm
    xrt
  )

  vitis_libs=(
    rt-engine
    target_factory
    unilog
    vart
    xir
  )

  for lib in ${vitis_debs[@]}; do
    lib_path=$(dpkg -L $lib | grep -F .so)
    echo "$lib_path" >> $DEPS_FILE
  done

  for lib in ${vitis_libs[@]}; do
    lib_path=$(grep -F .so /usr/local/manifests/$lib.txt)
    echo "$lib_path" >> $DEPS_FILE
  done

  # we need xrm in the production container. Manually copy over the needed files
  other_files=(
    /etc/systemd/system/xrmd.service
    /opt/xilinx/xrt/version.json
  )

  for file in ${other_files[@]}; do
    echo "$file" >> $DEPS_FILE
  done

  # any other binary dependencies needed
  other_files=(
    /opt/xilinx/xrm/bin/xrmd
    /bin/netstat
  )

  for bin in ${other_files[@]}; do
    get_dependencies $bin
    resolve_symlinks $bin
  done
}

add_migraphx_deps() {
  # we need these files in the production container. Manually copy over
  other_files=(
    /usr/share/libdrm/amdgpu.ids
  )

  for file in ${other_files[@]}; do
    echo "$file" >> $DEPS_FILE
  done

  other_bins=(
    /opt/rocm-5.0.0/llvm/bin/clang
  )

  for bin in ${other_bins[@]}; do
    get_dependencies $bin
    resolve_symlinks $bin
  done

  # something is missing in migraphx... for now, add everything
  all_libs=$(find /opt/rocm/ -name *.so*)

  for file in ${all_libs[@]}; do
    echo "$file" >> $DEPS_FILE
  done
}

add_other_bins() {
  # any other binary dependencies needed

  other_files=(
    /bin/systemctl
  )

  for bin in ${other_files[@]}; do
    get_dependencies $bin
    resolve_symlinks $bin
  done
}

remove_duplicates() {
  cat -n $DEPS_FILE | sort -uk2 | sort -nk1 | cut -f2- > $UNIQUE_DEPS_FILE
  rm $DEPS_FILE
}

copy_files() {
  cat $UNIQUE_DEPS_FILE | xargs -I{} bash -c "if [ -f {} ]; then cp --parents -P {} $1; fi"
}

parse_path() {
  path=$(realpath "$1")

  if ! compgen -G "$path" &> /dev/null; then
    echo "$path does not exist, skipping"
    return
  fi

  dynamic_files=( $(file $path | awk -F: '$2 ~ "dynamically linked" {print $1}') )
  for dynamic_file in "${dynamic_files[@]}"; do
    if [[ ! -z $dynamic_file ]]; then
      get_dependencies $dynamic_file
    fi
  done
}

COPY=""
VITIS=""
MIGRAPHX=""

# Parse Options
while true; do
  if [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
    usage;
    exit 0;
  fi
  if [[ -z "$1" ]]; then
    break;
  fi
  case "$1" in
    "-c" | "--copy" ) COPY=$2    ; shift 2 ;;
    "--vitis"       ) VITIS=$2   ; shift 2 ;;
    "--migraphx"    ) MIGRAPHX=$2; shift 2 ;;
    *) break;;
  esac
done

# overwrite the file
echo -n > $DEPS_FILE

paths=($(cat /usr/local/manifests/proteus.txt))

if [[ $VITIS == "yes" ]]; then
  paths+=("./external/aks/libs/*")
fi

for path in "${paths[@]}"; do
  parse_path "$path"
done

if [[ $VITIS == "yes" ]]; then
  add_vitis_deps
fi

if [[ $MIGRAPHX == "yes" ]]; then
  add_migraphx_deps
fi

add_other_bins

remove_duplicates

if [[ ! -z $COPY ]]; then
  copy_files $COPY
else
  cat $UNIQUE_DEPS_FILE
  rm $UNIQUE_DEPS_FILE
fi
