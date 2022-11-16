#!/usr/bin/env bats
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

setup_file() {
    # get the containing directory of this file
    # use $BATS_TEST_FILENAME instead of ${BASH_SOURCE[0]} or $0,
    # as those will point to the bats executable's location or the preprocessed
    # file respectively
    DIR="$( cd "$( dirname "$BATS_TEST_FILENAME" )" >/dev/null 2>&1 && pwd )"
    # make executables in AMDINFER_ROOT visible to PATH
    PATH="$DIR/../..:$PATH"
}

setup() {
    load "test_helper/bats-support/load"
    load "test_helper/bats-assert/load"
    load "test_helper/bats-file/load"

    source ./docker/get_dynamic_dependencies.sh
}

@test "runnable" {
    ./docker/get_dynamic_dependencies.sh
}

@test "save_data_echo" {
    run save_data "foobar"
    assert_output "foobar"
}

@test "save_data_file" {
    export DEPS_FILE=/tmp/deps.txt
    run save_data "foobar"
    assert_file_exists /tmp/deps.txt
    run cat /tmp/deps.txt
    assert_output "foobar"
    rm $DEPS_FILE
}

@test "resolve_symlinks" {
    run resolve_symlinks "/lib/x86_64-linux-gnu/libpthread.so.0"
    assert_line --index 0 "/lib/x86_64-linux-gnu/libpthread.so.0"
    assert_line --index 1 "/lib/x86_64-linux-gnu/libpthread-2.27.so"
}

@test "get_dependencies_text" {
    run get_dependencies "/usr/lib/os-release"
    assert_output "/usr/lib/os-release"
}

@test "get_dependencies_lib" {
    run get_dependencies "/lib/x86_64-linux-gnu/libpthread-2.27.so"
    assert_output
}

@test "find_dynamic_dir" {
    run find_dynamic "/lib/x86_64-linux-gnu/"
    assert_output
}

@test "find_dynamic_file" {
    run find_dynamic "/etc/os-release"
    refute_output
}

@test "find_dynamic_exe" {
    run find_dynamic "/bin/echo"
    refute_output
}
