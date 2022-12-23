#!/usr/bin/env bash
# Copyright 2022 Xilinx, Inc.
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

tool=""
run=0
check=0
clean=0

usage_lint_cache() {
cat << EOF
Run linting tools with caching

usage: ./tools/lint_cache.sh [flags]

flags: provide options for this command
  --tool      - tool to run: iwyu or tidy
  --run       - run the specified tool
  --check     - print the results from the specified tool
  -p          - max number of processes to use. Defaults to nproc / 2
  -h | --help - prints this message and exits
EOF
}

# arbitrarily use half the available processes to run linting
max_processes=$(( $(nproc) / 2 ))

while true
do
  case "$1" in
    --tool      ) tool="$2"        ; shift 2 ;;
    --run       ) run=1            ; shift 1 ;;
    --check     ) check=1          ; shift 1 ;;
    --clean     ) clean=1          ; shift 1 ;;
    -p          ) max_processes=$2 ; shift 2 ;;
    -h | --help ) usage_lint_cache ; exit  0 ;;
    *) break ;;
  esac
done

if [[ $run == 0 && $check == 0 ]]; then
    echo "At least one of --run or --check must be set"
    exit 1
fi

if [[ "$tool" != "iwyu" && "$tool" != "tidy" ]]; then
    echo "--tool must be set to iwyu or tidy"
    exit 1
fi

run() {
    source_file="$1"
    linter="$2"
    new_config_hash="$3"
    cmd="$4"

    echo $source_file
    # strip the root_dir from the source file path
    relative_path=${source_file##${root_dir}/}
    cache_entry="$cache_dir/$linter/$relative_path"
    new_hash=$(md5sum $source_file | cut -d ' ' -f 1)
    if [[ -f "$cache_entry" && $clean != 1 ]]; then
        old_hash=$(tail -n 1 $cache_entry)
        old_config_hash=$(tail -n 2 $cache_entry | head -n 1)
        if [[ $old_hash == $new_hash && $old_config_hash == $new_config_hash ]]; then
            return
        fi
    fi
    mkdir -p $(dirname $cache_entry)
    $cmd > $cache_entry 2>&1
    echo $new_config_hash >> $cache_entry
    echo $new_hash >> $cache_entry
}

run_iwyu() {
    python3 /usr/local/bin/iwyu_tool.py -p ${build_dir} $source_file -- -Xiwyu --mapping_file=$root_dir/tools/.iwyu.json
}

run_tidy() {
    clang-tidy -format-style=file -p=${build_dir} --extra-arg "-DCV_STATIC_ANALYSIS=0" $source_file
}

check() {
    source_file="$1"
    linter="$2"

    # strip the root_dir from the source file path
    relative_path=${source_file##${root_dir}/}
    cache_entry="$cache_dir/$linter/$relative_path"
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

root_dir="$AMDINFER_ROOT"
if [[ -z "$root_dir" ]]; then
    echo "AMDINFER_ROOT isn't set in the env"
    exit 1
fi

build_dir="$root_dir/build/Debug"
if [[ ! -d $build_dir ]]; then
    echo "The debug build directory doesn't exist, run 'amdinfer build --debug' once"
    exit 1
fi

tidy_hash=$(md5sum $root_dir/.clang-tidy | cut -d ' ' -f 1)
iwyu_hash=$(md5sum $root_dir/tools/.iwyu.json | cut -d ' ' -f 1)

cache_dir="$build_dir/lint_cache"
mkdir -p "$cache_dir"

echo "Finding source files..."
source_files=()
for directory in ${source_directories[@]}; do
    for extension in ${extensions[@]}; do
        # $() strips the trailing newline, so use trick to get it back
        source_files_x=$(find "$root_dir"/${directory} -path "*/build/*" -prune -o -name "*$extension" -print; echo x)
        source_files+="${source_files_x%x}"
    done
done

process_counter=0
for source_file in ${source_files[@]}; do
    if [[ $run == 1 ]]; then
        if [[ "$tool" == "iwyu" ]]; then
            run $source_file "iwyu" $iwyu_hash run_iwyu  &
        fi
        if [[ "$tool" == "tidy" ]]; then
            run "$source_file" "tidy" $tidy_hash run_tidy &
        fi
        process_counter=$(( process_counter + 1 ))
        if [ "$process_counter" -ge "$max_processes" ]; then
            wait
            process_counter=0
        fi
    fi
    if [[ $check == 1 ]]; then
        if [[ "$tool" == "iwyu" ]]; then
            check $source_file iwyu
        fi
        if [[ "$tool" == "tidy" ]]; then
            check $source_file tidy
        fi
    fi
done

wait
