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

print_banner(){
echo -e \
"\e[1;90m
=================================================
\e[m \e[1;91m
 ______
(_____ \                _
 _____) )  ____   ___  | |_    ____  _   _   ___
|  ____/  / ___) / _ \ |  _)  / _  )| | | | /___)
| |      | |    | |_| || |__ ( (/ / | |_| ||___ |
|_|      |_|     \___/  \___) \____) \____|(___/

\e[m \e[1;90m
=================================================
\e[m"

}

# ~/.bashrc: executed by bash(1) for non-login shells.
# see /usr/share/doc/bash/examples/startup-files (in the package bash-doc)
# for examples

# If not running interactively, don't do anything
[ -z "$PS1" ] && return

# don't put duplicate lines in the history. See bash(1) for more options
# ... or force ignoredups and ignorespace
HISTCONTROL=ignoredups:ignorespace

# append to the history file, don't overwrite it
shopt -s histappend

# for setting history length see HISTSIZE and HISTFILESIZE in bash(1)
HISTSIZE=1000
HISTFILESIZE=2000

# check the window size after each command and, if necessary,
# update the values of LINES and COLUMNS.
shopt -s checkwinsize

# make less more friendly for non-text input files, see lesspipe(1)
# [ -x /usr/bin/lesspipe ] && eval "$(SHELL=/bin/sh lesspipe)"

# set variable identifying the chroot you work in (used in the prompt below)
# if [ -z "$debian_chroot" ] && [ -r /etc/debian_chroot ]; then
#     debian_chroot=$(cat /etc/debian_chroot)
# fi

# set a fancy prompt (non-color, unless we know we "want" color)
# case "$TERM" in
#     xterm-color) color_prompt=yes;;
# esac

# uncomment for a colored prompt, if the terminal has the capability; turned
# off by default to not distract the user: the focus in a terminal window
# should be on the output of commands, not on the prompt
force_color_prompt=yes

if [ -n "$force_color_prompt" ]; then
    if [ -x /usr/bin/tput ] && tput setaf 1 >&/dev/null; then
        # We have color support; assume it's compliant with Ecma-48
        # (ISO/IEC-6429). (Lack of such support is extremely rare, and such
        # a case would tend to support setf rather than setaf.)
        color_prompt=yes
    else
        color_prompt=
    fi
fi

if [ "$color_prompt" = yes ]; then
    PS1="\[\033[38;5;2m\]\h\[$(tput sgr0)\]:\[$(tput sgr0)\]\[\033[38;5;4m\]\W\[$(tput sgr0)\]\\$ \[$(tput sgr0)\]"
else
    PS1="\h:\W\\$ \[$(tput sgr0)\]"
fi
unset color_prompt force_color_prompt

# If this is an xterm set the title to user@host:dir
# case "$TERM" in
# xterm*|rxvt*)
#     PS1="\[\e]0;${debian_chroot:+($debian_chroot)}\u@\h: \w\a\]$PS1"
#     ;;
# *)
#     ;;
# esac

# enable color support of ls and also add handy aliases
if [ -x /usr/bin/dircolors ]; then
    test -r ~/.dircolors && eval "$(dircolors -b ~/.dircolors)" || eval "$(dircolors -b)"
    alias ls='ls --color=auto'
    alias grep='grep --color=auto'
    alias fgrep='fgrep --color=auto'
    alias egrep='egrep --color=auto'
fi

# some more ls aliases
alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'

# Add an "alert" alias for long running commands.  Use like so:
#   sleep 10; alert
alias alert='notify-send --urgency=low -i "$([ $? = 0 ] && echo terminal || echo error)" "$(history|tail -n1|sed -e '\''s/^\s*[0-9]\+\s*//;s/[;&|]\s*alert$//'\'')"'

# Alias definitions.
# You may want to put all your additions into a separate file like
# ~/.bash_aliases, instead of adding them here directly.
# See /usr/share/doc/bash-doc/examples in the bash-doc package.
if [ -f ~/.bash_aliases ]; then
    . ~/.bash_aliases
fi

# enable programmable completion features (you don't need to enable
# this, if it's already enabled in /etc/bash.bashrc and /etc/profile
# sources /etc/bash.bashrc).
if [ -f /etc/bash_completion ] && ! shopt -oq posix; then
    . /etc/bash_completion
fi

if [ -f ~/.env ]; then
    . ~/.env
fi

# Check for PT/TF in Inference Server to set environment
# variables accordingly

TFZENDNN_FOUND=0
PTZENDNN_FOUND=0

ldconfig -p | grep libtensorflow_cc >/dev/null 2>&1
if [ $? -eq 0 ]; then
    TFZENDNN_FOUND=1
fi

ldconfig -p | grep torch_cpu >/dev/null 2>&1
if [ $? -eq 0 ]; then
    PTZENDNN_FOUND=1
fi

if [ $TFZENDNN_FOUND -eq 1 ] || [ $PTZENDNN_FOUND -eq 1 ]; then
    source ${PROTEUS_ROOT}/external/zendnn/zendnn_tf_pt_cc_api_env_setup.sh
fi

clear
print_banner

if [ $TFZENDNN_FOUND -eq 1 ]; then
    echo "TF+ZenDNN found."
    ZENDNN_FOUND=1
fi

if [ $PTZENDNN_FOUND -eq 1 ]; then
    echo "PT+ZenDNN found."
fi

if [ $TFZENDNN_FOUND -eq 1 ] || [ $PTZENDNN_FOUND -eq 1 ]; then
    echo "Please set below environment variables explicitly as per the platform you are using!!"
    echo -e "\tOMP_NUM_THREADS, GOMP_CPU_AFFINITY"
    echo "Please refer to documentation available at developer.amd.com/zendnn for performance"
fi
