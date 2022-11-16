#!/bin/sh
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

# Instructions:
#   1. git clone <url to Proteus repo>
#   2. cd amdinfer
#   4. ./tools/autotest.sh 2>&1 | tee test.txt

# print an easily searchable header so we can navigate the output. Search for
# "+++---" in the output text
print_header(){
  len=$(echo -n $1 | wc -m)
  len="$((len-3))" # leave room for the +++

  echo ""
  printf "+++"
  printf '%0.s-' $(seq 1 $len)
  echo ""
  echo "$1"
  printf "+++"
  printf '%0.s-' $(seq 1 $len)
  echo ""
  echo ""
}

# end the script if any command has a non-zero return value
set -e

print_header "Verifying autotesting prerequisites"

# assert that docker, docker-compose and git are all present
docker=$(which docker)
docker_compose=$(which docker-compose)
git=$(which git)

# log the versions of the prerequisites
docker_version=$($docker --version | awk -F ' ' '{print $3}' | sed 's/\(.*\),/\1 /')
docker_compose_version=$($docker_compose --version | awk -F ' ' '{print $3}' | sed 's/\(.*\),/\1 /')
git_version=$($git --version | awk -F ' ' '{print $3}')
echo "Docker version: $docker_version"
echo "Docker-Compose version: $docker_compose_version"
echo "Git version: $git_version"

print_header "Initializing repository"

# log the amdinfer version, git commit and make sure amdinfer exists
amdinfer_version=$(cat VERSION)
echo "Proteus version: $amdinfer_version"
commit=$(git rev-parse HEAD)
echo "Git commit: $commit"
test -e ./amdinfer

print_header "Building the stable dev docker image"

# use the --dry-run flag to log the "docker build" commands used
./amdinfer --dry-run dockerize
./amdinfer dockerize

print_header "Building the stable production docker image"

./amdinfer --dry-run dockerize --production
./amdinfer dockerize --production

# These commands add <user>/amdinfer-dev:<amdinfer-version> and
# <user>/amdinfer:<amdinfer-version> to the local machine. Append
# "--build-meta <build num>" to the commands above to change the image tags to
# <user>/amdinfer[-dev]:<amdinfer-version>+<build num>. They also update the
# latest tags on these images

print_header "Testing the stable dev docker image"

./amdinfer --dry-run up --profile autotest-dev
./amdinfer up --profile autotest-dev --write-only
docker-compose stop
docker-compose rm -f
./amdinfer up --profile autotest-dev

# This test must be run after the previous one. It requires that certain files
# e.g. the AKS files exist in the repository. These files are created when
# the project is built in the autotest-dev test.
print_header "Testing the stable production docker image"

./amdinfer --dry-run up --profile autotest
./amdinfer up --profile autotest --write-only
docker-compose stop
docker-compose rm -f
./amdinfer up --profile autotest
