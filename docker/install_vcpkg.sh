#!/usr/bin/env bash
# Copyright 2023 Advanced Micro Devices, Inc.
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

set -eo pipefail

while true
do
  case "$1" in
    --vitis       ) VITIS=$2      ; shift 2 ;;
    --rocal       ) ROCAL=$2      ; shift 2 ;;
    *) break ;;
  esac
done

# for some reason, vcpkg doesn't like passing an empty string as an argument
# so set it to a real value when VITIS is "no"

FEATURES="--x-feature=testing"

if [[ "$VITIS" == "yes" ]]; then
  FEATURES="$FEATURES --x-feature=vitis"
fi

if [[ "$ROCAL" == "yes" ]]; then
  FEATURES="$FEATURES --x-feature=rocal"
fi

/opt/vcpkg/vcpkg/vcpkg install --x-install-root=/opt/vcpkg --triplet=x64-linux-dynamic --clean-after-build $FEATURES
