#!/bin/sh
# Copyright 2021 Xilinx, Inc.
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

user="$1"
shift

# start the XRM daemon if it exists and if the XRM port isn't in use
if systemctl --all --type service | grep -q xrmd; then
  xrm_alive=$(netstat -tulpn | grep :9763)
  if [ -z "$xrm_alive" ] ; then
    systemctl start xrmd
  fi
fi

# if the user has mounted any files/directories into /workspace, get the UID/GID
# from the first found one and use it to adjust the UID/GID of amdinfer-user
# Changing UID/GID taken from https://stackoverflow.com/a/46057716
for dir in /workspace/*; do  # lists the absolute path to the file/directory
  if [ ! -d "$dir" ] && [ ! -f "$dir" ]; then
    continue
  fi

  # get uid/gid
  USER_UID=$(ls -nd $dir | cut -f3 -d' ')
  USER_GID=$(ls -nd $dir | cut -f4 -d' ')

  # get the current uid/gid of amdinfer-user
  CUR_UID=$(getent passwd amdinfer-user | cut -f3 -d: || true)
  CUR_GID=$(getent group amdinfer-user | cut -f3 -d: || true)

  # if we're mounting the amdinfer repo, copy over the bash files at runtime
  if [ -d /workspace/amdinfer/docker ]; then
    cp -f /workspace/amdinfer/docker/.bash* /home/amdinfer-user
  fi

  # if they don't match, adjust
  if [ -n "$USER_GID" ] && [ "$USER_GID" != "$CUR_GID" ]; then
    groupmod -g "${USER_GID}" amdinfer
    chown -R --silent :amdinfer /home/amdinfer-user
  fi
  if [ -n "$USER_UID" ] && [ "$USER_UID" != "$CUR_UID" ]; then
    usermod -u "${USER_UID}" amdinfer-user
    # fix other permissions
    chown -R --silent amdinfer-user /home/amdinfer-user
  fi

  break
done

# Any devices passed into the container are assumed to be in /dev/. Then, we
# want to allow the container user to have access to them. So, we find all the
# GIDs that own devices in /dev/, create groups for them if they don't exist,
# and add the user to these groups
groups=$(find /dev/ | xargs stat -c %g | sort | uniq)

# used to create generic new group names
count=0
new_group="group$count"

for group in $groups; do
  # exclude the root (0) and tty (5) group IDs
  if [ $group -ne 0 ] && [ $group -ne 5 ]; then
    # if the GID doesn't exist, then add it
    if ! getent group $group > /dev/null; then
      groupadd -g $group $new_group
      group=$new_group

      count=$((count+1))
      new_group="group$count"
    fi
    # if the user isn't a member of the group, then join it
    if ! id -nG amdinfer-user | grep -qw "$group"; then
      usermod -aG $group amdinfer-user
      usermod -aG $group root
    fi
  fi
done

# insert line break
echo ""

# if there are any FPGAs, attempt to load xclbins
if command -v fpga-util >/dev/null 2>&1; then
  fpga-util load-all
fi

# drop access to the right user and run the CMD
if [ "$user" = "root" ]; then
  exec gosu root "$@"
else
  exec gosu amdinfer-user "$@"
fi
