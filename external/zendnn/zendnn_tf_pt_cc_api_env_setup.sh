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

export ZENDNN_LOG_OPTS=ALL:0
echo "ZENDNN_LOG_OPTS=$ZENDNN_LOG_OPTS"

export OMP_WAIT_POLICY=ACTIVE
echo "OMP_WAIT_POLICY=$OMP_WAIT_POLICY"

export OMP_PROC_BIND=FALSE
echo "OMP_PROC_BIND=$OMP_PROC_BIND"

#By default setting ZENDNN_TF_VERSION as 2.10
export ZENDNN_TF_VERSION=2.10
echo "ZENDNN_TF_VERSION=$ZENDNN_TF_VERSION"

#By default setting ZENDNN_PT_VERSION as v1.12.0
export ZENDNN_PT_VERSION="1.12.0"
echo "ZENDNN_PT_VERSION=$ZENDNN_PT_VERSION"

#If the environment variable OMP_DYNAMIC is set to true, the OpenMP implementation
#may adjust the number of threads to use for executing parallel regions in order
#to optimize the use of system resources. ZenDNN depend on a number of threads
#which should not be modified by runtime, doing so can cause incorrect execution
export OMP_DYNAMIC=FALSE
echo "OMP_DYNAMIC=$OMP_DYNAMIC"

#Disable TF check for training ops and stop execution if any training ops
#found in TF graph. By default, its enabled
export ZENDNN_INFERENCE_ONLY=1
echo "ZENDNN_INFERENCE_ONLY=$ZENDNN_INFERENCE_ONLY"

#Disable TF memory pool optimization, By default, its enabled
export ZENDNN_ENABLE_MEMPOOL=1
echo "ZENDNN_ENABLE_MEMPOOL=$ZENDNN_ENABLE_MEMPOOL"

#Set the max no. of tensors that can be used inside TF memory pool, Default is
#set to 32
export ZENDNN_TENSOR_POOL_LIMIT=32
echo "ZENDNN_TENSOR_POOL_LIMIT=$ZENDNN_TENSOR_POOL_LIMIT"

#Enable fixed max size allocation for Persistent tensor with TF memory pool
#optimization, By default, its disabled
export ZENDNN_TENSOR_BUF_MAXSIZE_ENABLE=0
echo "ZENDNN_TENSOR_BUF_MAXSIZE_ENABLE=$ZENDNN_TENSOR_BUF_MAXSIZE_ENABLE"

# Convolution GEMM path is set as defualt
export ZENDNN_CONV_ALGO=1
echo "ZENDNN_CONV_ALGO=$ZENDNN_CONV_ALGO"

# INT8 support  is disabled by default
export ZENDNN_INT8_SUPPORT=0
echo "ZENDNN_INT8_SUPPORT=$ZENDNN_INT8_SUPPORT"

# ZENDNN_GEMM_ALGO is set to 3 by default
export ZENDNN_GEMM_ALGO=3
echo "ZENDNN_GEMM_ALGO=$ZENDNN_GEMM_ALGO"

# INT8 support  is disabled by default
export ZENDNN_INT8_SUPPORT=0
echo "ZENDNN_INT8_SUPPORT=$ZENDNN_INT8_SUPPORT"

# Switch to enable Conv, Add fusion on users discretion. Currently it is
# safe to enable this switch for resnet50v1_5, resnet101, and
# inception_resnet_v2 models only. By default the switch is disabled.
export ZENDNN_TF_CONV_ADD_FUSION_SAFE=0
echo "ZENDNN_TF_CONV_ADD_FUSION_SAFE=$ZENDNN_TF_CONV_ADD_FUSION_SAFE"

#optimize_for_inference has to be called only after setting this flag for this fusion to happen
export ZENDNN_PT_CONV_ADD_FUSION_SAFE=0
echo "ZENDNN_PT_CONV_ADD_FUSION_SAFE=$ZENDNN_PT_CONV_ADD_FUSION_SAFE"

# Primitive reuse is disabled by default
export TF_ZEN_PRIMITIVE_REUSE_DISABLE=FALSE
echo "TF_ZEN_PRIMITIVE_REUSE_DISABLE=$TF_ZEN_PRIMITIVE_REUSE_DISABLE"

# Primitive Caching Capacity
export ZENDNN_PRIMITIVE_CACHE_CAPACITY=1024
echo "ZENDNN_PRIMITIVE_CACHE_CAPACITY: $ZENDNN_PRIMITIVE_CACHE_CAPACITY"

# Enable primitive create and primitive execute logs. By default it is disabled
export ZENDNN_PRIMITIVE_LOG_ENABLE=0
echo "ZENDNN_PRIMITIVE_LOG_ENABLE: $ZENDNN_PRIMITIVE_LOG_ENABLE"

sleep 1
