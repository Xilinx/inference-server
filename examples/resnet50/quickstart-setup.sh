#!/bin/bash

## Quick Start setup for Server Deployment
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


## Quick Start setup for Client Inference
wget https://github.com/Xilinx/inference-server/raw/main/examples/resnet50/imagenet_classes.txt
wget https://github.com/Xilinx/inference-server/raw/main/tests/assets/dog-3619020_640.jpg
wget https://github.com/Xilinx/inference-server/raw/main/examples/resnet50/resnet.py
wget https://github.com/Xilinx/inference-server/raw/main/examples/resnet50/tfzendnn.py
