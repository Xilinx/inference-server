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

set -e

usage() {
cat << EOF
usage: ./test.sh [--mode MODE] [mode args]

Args:
  --mode <python|cpp|examples|all>: choose which tests to run. Defaults to python
  --load-before: attempt to load xclbins to all unloaded FPGAs before running tests

Python args:
  -k <STR>: run specific tests matching this substring
  -s: disable capturing stdout in pytest. Set this option to enable printing
  --hostname <STR>: hostname of Proteus server. Defaults to localhost
  --http_port <NUM>: port to use for Proteus's HTTP server. Defaults to Proteus's default value
  --fpgas name:num,...: type and number of FPGAs to use for testing. Autodetects by default
  --benchmark <skip|only|all>: skip or only run benchmark tests, or run everything. Defaults to skip

CPP args:
  --build <Debug|Coverage>: build to test. Defaults to Debug
EOF
}

MODE="python"
LOAD=""

FPGAS=""
HOSTNAME="localhost"
PORT=""
TESTS=""
CAPTURE=""
BENCHMARK="skip"
SAVE_BENCHMARK=""

BUILD="Debug"

full_path=$(realpath $0)
python_tests_path=$(dirname $full_path)/python
examples_path=$(dirname $full_path)/../examples
cur_path=$(pwd)

while true
do
  case "$1" in
    --benchmark       ) BENCHMARK=$2     ; shift 2 ;;
    --hostname        ) HOSTNAME="$2"    ; shift 2 ;;
    --http_port       ) PORT="$2"        ; shift 2 ;;
    --fpgas           ) FPGAS=$2         ; shift 2 ;;
    --load-before     ) LOAD="1"         ; shift 1 ;;
    --mode            ) MODE="$2"        ; shift 2 ;;
    -k                ) TESTS="$2"       ; shift 2 ;;
    -s                ) CAPTURE="-s"     ; shift 1 ;;
    --build           ) BUILD="$2"       ; shift 2 ;;
    -h | --help       ) usage            ; exit  0 ;;
    *) break ;;
  esac
done

retval=0
if [[ $MODE == "python" || $MODE == "all" ]]; then

  if [[ $PORT != "" ]]; then
    PORT="--http_port $PORT"
  fi

  if [[ ! -z $FPGAS ]]; then
    FPGAS="--fpgas $FPGAS"
  fi

  if [[ $BENCHMARK == "only" ]]; then
    SAVE_BENCHMARK="--benchmark-autosave"
  fi

  if [[ -z $LD_LIBRARY_PATH && -f ~/.env ]]; then
    source ~/.env
  fi

  if [[ -n "$LOAD" ]]; then
    fpga-util load-all
  fi

  cd "$python_tests_path"
  if [[ -n $TESTS ]]; then
    pytest $CAPTURE -ra --hostname $HOSTNAME $PORT $SAVE_BENCHMARK \
      $FPGAS --benchmark $BENCHMARK --benchmark-quiet -k "$TESTS"
    retval=$(($retval | $?))
  else
    pytest $CAPTURE -ra --hostname $HOSTNAME $PORT $SAVE_BENCHMARK \
      $FPGAS --benchmark $BENCHMARK --benchmark-quiet
    retval=$(($retval | $?))
  fi

  cd "$cur_path"
fi

if [[ $MODE == "cpp" || $MODE == "all" ]]; then
  # ctest fails to detect the tests in Coverage build for some reason
  # ctest --test-dir $PROTEUS_ROOT/build/$BUILD/tests/cpp/ --output-on-failure
  # TODO(varunsh): facedetect is excluded while it's being debugged
  $PROTEUS_ROOT/build/$BUILD/tests/cpp/test_gtest --gtest_filter=-Native.Facedetect
  retval=$(($retval | $?))
fi

if [[ $MODE == "examples" || $MODE == "all" ]]; then
  for file in $(find $examples_path/python -type f -print); do
    if [[ $file == *.py ]]; then
      python3 $(realpath $file)
    fi
  done

  for file in $(find $PROTEUS_ROOT/build/$BUILD/examples/cpp -type f -print); do
    real_file=$(realpath $file)
    if [ -x $real_file ]; then
      $real_file
    fi
  done
fi

exit $retval
