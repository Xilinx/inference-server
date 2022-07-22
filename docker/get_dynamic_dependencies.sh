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

# This script will find, and optionally copy, all the dependencies needed to run
# the inference server in the release production container.

usage() {
cat << EOF
./get_dynamic_dependencies.sh [--copy /path/] [flags]

--copy: copy the found files to the provided location. If not provided, just
        print to stdout

flags:
  --vitis [yes|no]: Add Vitis-related dependencies
  --migraphx [yes|no]: Add migraphx-related dependencies
EOF
}

ALL_ARGS=("$@")
# get the current directory
# DIR="$( cd "$( dirname "$0" )" && pwd )"

save_data() {
  if [[ -n $DEPS_FILE ]]; then
    printf '%s\n' "$1" >> $DEPS_FILE
  else
    printf '%s\n' "$1"
  fi
}

# Usage: resolve_symlinks /path/to/file
#
# This function will add the file, and any symlinks along the way, to the
# dependency list with absolute paths
resolve_symlinks() {
  dep=$1

  dir=$(dirname $dep)
  while [[ -L $dep ]]; do
    save_data "$(realpath -s $dep)"
    dep=$(readlink $dep)
    # if the next path isn't absolute, append the directory from above and
    # resolve full path
    if [[ ! "$dep" = /* ]]; then
      dep="$dir/$dep"
    fi
  done
  save_data "$(realpath -s $dep)"
}

# Usage: get_dependencies /path/to/file
#
# This function only works for dynamically linked objects (other files are
# ignored). The shared libraries the file links to (with ldd) are added to
# the dependency list.
get_dependencies() {
  dynamic_file=$1

  # extract just the paths to the shared libraries
  readarray -t deps < <(ldd $dynamic_file | tr -s '[:blank:]' '\n' | grep '^/')

  for dep in "${deps[@]}"; do
    resolve_symlinks $dep
  done
  # add the dynamic file itself
  resolve_symlinks $dep
}

# Usage: find_dynamic /path/to/dir
#
# Given a directory, find all the dynamic files in the tree and add them to the
# dependency list
find_dynamic() {
  path=$(realpath "$1")

  if [ ! -d $path ]; then
    return
  fi

  files=( $(find $path ))
  for file in "${files[@]}"; do
    get_dependencies $dynamic_file
  done
}

remove_duplicates() {
  cat -n $DEPS_FILE | sort -uk2 | sort -nk1 | cut -f2- > $UNIQUE_DEPS_FILE
  rm $DEPS_FILE
}

copy_files() {
  cat $UNIQUE_DEPS_FILE | xargs -I{} bash -c "if [ -f {} ]; then cp --parents -P {} $1; fi"
}

add_vitis_deps() {
  # some vitis libraries are found in .xmodels or are otherwise not linked to.
  # As such, we don't find them in get_dependencies. We add them here manually

  vitis_debs=(
    xrm
    xrt
  )

  vitis_manifests=(
    rt-engine
    target_factory
    unilog
    vart
    xir
  )

  for deb in ${vitis_debs[@]}; do
    lib_paths=($(dpkg -L $deb | grep -F .so))
    for lib in ${lib_paths[@]}; do
      get_dependencies $lib
    done
  done

  for manifest in ${vitis_manifests[@]}; do
    lib_paths=$(grep -F .so /usr/local/manifests/$manifest.txt)
    for lib in ${lib_paths[@]}; do
      get_dependencies $lib
    done
  done

  # we need xrm in the production container. Manually copy over the needed files
  other_files=(
    /etc/systemd/system/xrmd.service
    /opt/xilinx/xrt/version.json
  )

  for file in ${other_files[@]}; do
    get_dependencies $file
  done

  # any other binary dependencies needed
  other_files=(
    /opt/xilinx/xrm/bin/xrmd
    /bin/netstat
  )

  for bin in ${other_files[@]}; do
    get_dependencies $bin
  done
}

add_migraphx_deps() {
  # we need these files in the production container. Manually copy over
  other_files=(
    /usr/share/libdrm/amdgpu.ids
  )

  for file in ${other_files[@]}; do
    get_dependencies $file
  done

  other_bins=(
    /bin/lsmod
    /opt/rocm-5.0.0/llvm/bin/clang
  )

  for bin in ${other_bins[@]}; do
    get_dependencies $bin
  done

  # something is missing in migraphx... just adding .so files doesn't work
  # either so for now, add everything
  # all_libs=$(find /opt/rocm/ -name *.so*)
  all_files=$(find /opt/rocm/)

  for file in ${all_files[@]}; do
    resolve_symlinks $file
  done
}

add_other_bins() {
  # any other binary dependencies needed

  other_files=(
    /bin/systemctl
  )

  for bin in ${other_files[@]}; do
    get_dependencies $bin
  done
}

add_proteus_deps() {
  if test -f /usr/local/manifests/proteus.txt; then
    files=($(cat /usr/local/manifests/proteus.txt))
  elif test -f /root/deps/usr/local/manifests/proteus.txt; then
    files=($(cat /root/deps/usr/local/manifests/proteus.txt))
  elif test -f /tmp/proteus/build/Release/install_manifest.txt; then
    files=($(cat /tmp/proteus/build/Release/install_manifest.txt))
  else
    echo "Warning: no manifest file found"
    return
  fi

  for file in "${files[@]}"; do
    get_dependencies "$file"
  done
}

main() {
  # these files hold the temporary files as they're discovered
  DEPS_FILE=/tmp/deps.txt
  UNIQUE_DEPS_FILE=/tmp/deps_unique.txt

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

  add_proteus_deps

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
}

# if the script is being sourced, return. Otherwise, suppress the error from
# return and continue to call main()
return 2> /dev/null

main "$@"
