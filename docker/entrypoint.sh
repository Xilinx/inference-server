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


# start the XRM daemon if it exists and if the XRM port isn't in use
if systemctl --all --type service | grep -q xrmd; then
  xrm_alive=$(netstat -tulpn | grep :9763)
  if [ -z "$xrm_alive" ] ; then
    systemctl start xrmd
  fi
fi

# if the user has mounted any directories into /workspace, get the UID/GID from
# the first found directory and use it to adjust the UID/GID of proteus-user
# Changing UID/GID taken from https://stackoverflow.com/a/46057716
for dir in /workspace/*/; do  # list directories in the form "/workspace/dir/"
  if [ ! -d $dir ]; then
    continue
  fi

  # get uid/gid
  USER_UID=$(ls -nd $dir | cut -f3 -d' ')
  USER_GID=$(ls -nd $dir | cut -f4 -d' ')

  # get the current uid/gid of proteus-user
  CUR_UID=$(getent passwd proteus-user | cut -f3 -d: || true)
  CUR_GID=$(getent group proteus-user | cut -f3 -d: || true)

  # if we're mounting the proteus repo, copy over the bash files at runtime
  if [ -d /workspace/proteus/docker ]; then
    cp -f /workspace/proteus/docker/.bash* /home/proteus-user
  fi

  # if they don't match, adjust
  if [ -n "$USER_GID" ] && [ "$USER_GID" != "$CUR_GID" ]; then
    groupmod -g "${USER_GID}" proteus
    chown -R --silent :proteus /home/proteus-user
  fi
  if [ -n "$USER_UID" ] && [ "$USER_UID" != "$CUR_UID" ]; then
    usermod -u "${USER_UID}" proteus-user
    # fix other permissions
    chown -R --silent proteus-user /home/proteus-user
  fi

  break
done

# insert line break
echo ""

# drop access to proteus-user and run cmd
gosu proteus-user "$@"
