#!/usr/bin/env bash
# Copyright 2022 Xilinx Inc.
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

exit_trap () {
    local lc="$BASH_COMMAND" rc=$?
    if [[ $rc != 0 ]]; then
        echo "Command [$lc] exited with code [$rc]"
    fi
}

trap exit_trap EXIT

set -eo pipefail

# turn on extended globbing
shopt -s extglob

run=0
check=0

while true
do
  case "$1" in
    --run       ) run=1   ; shift 1 ;;
    --check     ) check=1 ; shift 1 ;;
    -h | --help ) usage   ; exit  0 ;;
    *) break ;;
  esac
done

if [[ $run == 0 && $check == 0 ]]; then
    echo "At least one of --run or --check must be set"
    exit 1
fi

run_tidy() {
    source_file="$1"

    echo $source_file
    # strip the root_dir from the source file path
    relative_path=${source_file##${root_dir}/}
    cache_entry="$cache_dir/$relative_path"
    new_hash=$(md5sum $source_file | cut -d ' ' -f 1)
    if [[ -f "$cache_entry" ]]; then
        old_hash=$(tail -n 1 $cache_entry)
        if [[ $old_hash == $new_hash ]]; then
            continue
        fi
    fi
    mkdir -p $(dirname $cache_entry)
    clang-tidy -format-style=file -p=${build_dir} --extra-arg "-DCV_STATIC_ANALYSIS=0" $source_file > $cache_entry 2>&1
    echo $new_hash >> $cache_entry
}

run_check() {
    source_file="$1"

    # strip the root_dir from the source file path
    relative_path=${source_file##${root_dir}/}
    cache_entry="$cache_dir/$relative_path"
    if [[ -f "$cache_entry" ]]; then
        if grep $source_file $cache_entry; then
            cat $cache_entry
        fi
    fi
}

# directories relative to the root to search for source files
source_directories=(examples include src tests)

# file extensions to search for
extensions=(".c" ".cpp" ".cc" ".h" ".hpp")

root_dir="$PROTEUS_ROOT"
if [[ -z "$root_dir" ]]; then
    echo "PROTEUS_ROOT isn't set in the env"
    exit 1
fi

build_dir="$root_dir/build/Debug"
if [[ ! -d $build_dir ]]; then
    echo "The debug build directory doesn't exist, run 'proteus build --debug' once"
    exit 1
fi

cache_dir="$build_dir/tidy_cache"
mkdir -p "$cache_dir"

source_files=()
for directory in ${source_directories[@]}; do
    for extension in ${extensions[@]}; do
        # $() strips the trailing newline, so use trick to get it back
        source_files_x=$(find "$root_dir"/${directory} -path "*/build/*" -prune -o -name "*$extension" -print; echo x)
        source_files+="${source_files_x%x}"
    done
done

for source_file in ${source_files[@]}; do
    if [[ $run == 1 ]]; then
        run_tidy $source_file
    fi
    if [[ $check == 1 ]]; then
        run_check $source_file
    fi
done
