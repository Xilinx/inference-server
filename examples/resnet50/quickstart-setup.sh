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

# This script automates downloading test files and setting up a model repository
# to run a sample inference. All files are downloaded in the current working
# directory.

# Download a TensorFlow ResNet50 model and create a model repository
wget -O tensorflow.zip https://www.xilinx.com/bin/public/openDownload?filename=tf_resnetv1_50_imagenet_224_224_6.97G_2.5.zip
unzip -j "tensorflow.zip" "tf_resnetv1_50_imagenet_224_224_6.97G_2.5/float/resnet_v1_50_baseline_6.96B_922.pb" -d .
mkdir -p model_repository/resnet50/1
mv ./resnet_v1_50_baseline_6.96B_922.pb model_repository/resnet50/1/saved_model.pb
echo 'name: "resnet50"
platform: "tensorflow_graphdef"
inputs [
    {
        name: "input"
        datatype: "FP32"
        shape: [224,224,3]
    }
]
outputs [
    {
        name: "resnet_v1_50/predictions/Reshape_1"
        datatype: "FP32"
        shape: [1000]
    }
]' > model_repository/resnet50/config.pbtxt


# Download the class labels for this model
wget https://github.com/Xilinx/inference-server/raw/main/examples/resnet50/imagenet_classes.txt
# Download a sample image
wget https://github.com/Xilinx/inference-server/raw/main/tests/assets/dog-3619020_640.jpg
# Download the example Python code
wget https://github.com/Xilinx/inference-server/raw/main/examples/resnet50/resnet.py
wget https://github.com/Xilinx/inference-server/raw/main/examples/resnet50/tfzendnn.py
