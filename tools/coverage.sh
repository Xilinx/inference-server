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

usage_coverage() {
cat << EOF
Run amdinfer's tests with coverage measurement.

usage: ./coverage.sh [flags]

flags: provide options for this command
  --get-artifacts              - get the xmodel artifacts
  -h | --help                  - prints this message and exits
  -t <num> | --threshold <num> - return non-zero if coverage is below threshold
EOF
}

coverage(){
  GET=""
  THRESHOLD=""

  while true; do
    if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
      usage_coverage;
      exit 0;
    fi
    if [ -z "$1" ]; then
      break;
    fi
    case "$1" in
      "--get-artifacts"     ) GET=1          ;      ;;
      "-t"  | "--threshold" ) THRESHOLD="$2" ; shift;;
      *)  echo "Unknown argument : $1";
          echo "Try ./amdinfer.sh coverage -h to get correct usage. Exiting ...";
          exit 1 ;;
    esac
    shift
  done

  if [[ -z $LD_LIBRARY_PATH && -f ~/.env ]]; then
    source ~/.env
  fi

  cd $PROTEUS_ROOT
  build_dir=${PROTEUS_ROOT}/build

  if [[ -n $GET ]]; then
    ./amdinfer get --all
  fi

  ./amdinfer build --coverage --regen --clean
  ./amdinfer make coverage

  cd -

  summary=$(lcov --summary $build_dir/Coverage/coverage.info)
  lines=$(echo "$summary" | grep "lines" | awk -F ' ' '{print $2}')
  lines=${lines%"%"} # remove trailing %
  functions=$(echo "$summary" | grep "functions" | awk -F ' ' '{print $2}')
  functions=${functions%"%"} # remove trailing %

  echo "$summary"

  if [[ -n $THRESHOLD ]]; then
    if awk "BEGIN {exit !($lines < $THRESHOLD)}"; then
      echo "Line coverage ($lines) is below threshold ($THRESHOLD)"
      return 1
    fi
    if awk "BEGIN {exit !($functions < $THRESHOLD)}"; then
      echo "Function coverage ($functions) is below threshold ($THRESHOLD)"
      return 2
    fi
  fi
}

coverage "$@"
