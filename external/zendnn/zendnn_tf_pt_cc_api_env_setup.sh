#!/usr/bin/env bash
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


#-----------------------------------------------------------------------------
#   zendnn_tf_pt_cc_api_env_setup.sh
#   Prerequisite: This script needs to run first to setup environment variables
#                 before before using the tf-zendnn or pt-zendnn c++ api library
#
#   This script does following:
#   -Checks if important env variables are declared
#   -Sets important environment variables for benchmarking
#----------------------------------------------------------------------------

#------------------------------------------------------------------------------

if [ -z "$ZENDNN_LOG_OPTS" ];
then
    export ZENDNN_LOG_OPTS=ALL:0
    echo "ZENDNN_LOG_OPTS=$ZENDNN_LOG_OPTS"
else
    echo "ZENDNN_LOG_OPTS=$ZENDNN_LOG_OPTS"
fi

if [ -z "$OMP_NUM_THREADS" ];
then
    export OMP_NUM_THREADS=64
    echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"
else
    echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"
fi

if [ -z "$OMP_WAIT_POLICY" ];
then
    export OMP_WAIT_POLICY=ACTIVE
    echo "OMP_WAIT_POLICY=$OMP_WAIT_POLICY"
else
    echo "OMP_WAIT_POLICY=$OMP_WAIT_POLICY"
fi

if [ -z "$OMP_PROC_BIND" ];
then
    export OMP_PROC_BIND=FALSE
    echo "OMP_PROC_BIND=$OMP_PROC_BIND"
else
    echo "OMP_PROC_BIND=$OMP_PROC_BIND"
fi

#By default setting ZENDNN_TF_VERSION as 2.7
if [ -z "$ZENDNN_TF_VERSION" ];
then
    export ZENDNN_TF_VERSION=2.7
    echo "ZENDNN_TF_VERSION=$ZENDNN_TF_VERSION"
else
    echo "ZENDNN_TF_VERSION=$ZENDNN_TF_VERSION"
fi

#If the environment variable OMP_DYNAMIC is set to true, the OpenMP implementation
#may adjust the number of threads to use for executing parallel regions in order
#to optimize the use of system resources. ZenDNN depend on a number of threads
#which should not be modified by runtime, doing so can cause incorrect execution
export OMP_DYNAMIC=FALSE
echo "OMP_DYNAMIC=$OMP_DYNAMIC"
export KMP_BLOCKTIME=200
echo "KMP_BLOCKTIME=$KMP_BLOCKTIME"

#Disable TF check for training ops and stop execution if any training ops
#found in TF graph. By default, its enabled
export ZENDNN_INFERENCE_ONLY=1
echo "ZENDNN_INFERENCE_ONLY=$ZENDNN_INFERENCE_ONLY"

#Disable TF memory pool optimization, By default, its enabled
export ZENDNN_MEMPOOL_ENABLE=1
echo "ZENDNN_MEMPOOL_ENABLE=$ZENDNN_MEMPOOL_ENABLE"

#Set the max no. of tensors that can be used inside TF memory pool, Default is
#set to 16
export ZENDNN_TENSOR_POOL_LIMIT=16
echo "ZENDNN_TENSOR_POOL_LIMIT=$ZENDNN_TENSOR_POOL_LIMIT"

#Enable fixed max size allocation for Persistent tensor with TF memory pool
#optimization, By default, its disabled
export ZENDNN_TENSOR_BUF_MAXSIZE_ENABLE=0
echo "ZENDNN_TENSOR_BUF_MAXSIZE_ENABLE=$ZENDNN_TENSOR_BUF_MAXSIZE_ENABLE"

# Blocked Format is disabled by default
export ZENDNN_BLOCKED_FORMAT=0
echo "ZENDNN_BLOCKED_FORMAT=$ZENDNN_BLOCKED_FORMAT"

# INT8 support  is disabled by default
export ZENDNN_INT8_SUPPORT=0
echo "ZENDNN_INT8_SUPPORT=$ZENDNN_INT8_SUPPORT"

# INT8 Relu6 fusion support is disabled by default
export ZENDNN_RELU_UPPERBOUND=0
echo "ZENDNN_RELU_UPPERBOUND=$ZENDNN_RELU_UPPERBOUND"

# Switch to enable Conv, Add fusion on users discretion. Currently it is
# safe to enable this switch for resnet50v1_5, resnet101, and
# inception_resnet_v2 models only. By default the switch is disabled.
export ZENDNN_TF_CONV_ADD_FUSION_SAFE=0
echo "ZENDNN_TF_CONV_ADD_FUSION_SAFE=$ZENDNN_TF_CONV_ADD_FUSION_SAFE"

# Primitive Caching Capacity
export ZENDNN_PRIMITIVE_CACHE_CAPACITY=1024
echo "ZENDNN_PRIMITIVE_CACHE_CAPACITY: $ZENDNN_PRIMITIVE_CACHE_CAPACITY"

# MAX_CPU_ISA
# MAX_CPU_ISA is disabld at build time. When feature is enabled, uncomment the
# below 2 lines
#export ZENDNN_MAX_CPU_ISA=ALL
#echo "ZENDNN_MAX_CPU_ISA: $ZENDNN_MAX_CPU_ISA"

# Enable primitive create and primitive execute logs. By default it is disabled
export ZENDNN_PRIMITIVE_LOG_ENABLE=0
echo "ZENDNN_PRIMITIVE_LOG_ENABLE: $ZENDNN_PRIMITIVE_LOG_ENABLE"

# Check if pt-zendnn is enabled, and if so, preload jemalloc and libomp
ldconfig -p | grep torch_cpu >/dev/null 2>&1
if [ $? -eq 0 ]; then
    ldconfig -p | grep libjemalloc.so >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        export LD_PRELOAD=/usr/local/lib/libjemalloc.so:$LD_PRELOAD
        export MALLOC_CONF="oversize_threshold:1,background_thread:true,metadata_thp:auto,dirty_decay_ms:-1,muzzy_decay_ms:-1"
        echo "PRELOADED JMALLOC"
    fi
    export LD_PRELOAD=/usr/lib/libomp.so:$LD_PRELOAD
fi

sleep 1
