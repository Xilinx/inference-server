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

import argparse
from pathlib import Path

from model import File, FloatOpenDownload, XModelOpenDownload


def get(args: argparse.Namespace):
    directory = Path("resnet50")

    models = {}
    if args.vitis:
        models["u250_resnet50"] = XModelOpenDownload(
            "resnet_v1_50_tf-u200-u250-r2.5.0.tar.gz",
            directory / "u250",
            "resnet_v1_50_tf/resnet_v1_50_tf.xmodel",
        )
        models["vck5000-4pe_resnet50"] = XModelOpenDownload(
            "resnet_v1_50_tf-vck5000-DPUCVDX8H-4pe-r2.5.0.tar.gz",
            directory / "vck5000",
            "resnet_v1_50_tf/resnet_v1_50_tf.xmodel",
        )
    if args.ptzendnn:
        models["pt_resnet50"] = FloatOpenDownload(
            "pt_resnet50_imagenet_224_224_8.2G_2.5.zip",
            directory,
            "pt_resnet50_imagenet_224_224_8.2G_2.5/float/resnet50_pretrained.pth",
        )
    if args.tfzendnn:
        models["tf_resnet50"] = FloatOpenDownload(
            "tf_resnetv1_50_imagenet_224_224_6.97G_2.5.zip",
            directory,
            "tf_resnetv1_50_imagenet_224_224_6.97G_2.5/float/resnet_v1_50_baseline_6.96B_922.pb",
        )
    if args.migraphx:
        models["onnx_resnet50"] = File(
            "https://github.com/onnx/models/raw/5faef4c33eba0395177850e1e31c4a6a9e634c82/vision/classification/resnet/model/resnet50-v2-7.onnx",
            directory,
            "",
        )
        models["onnx_resnet50_val"] = File(
            "https://github.com/mvermeulen/rocm-migraphx/raw/master/datasets/imagenet/val.txt",
            directory,
            "",
        )
        models["onnx_resnet50_fp16"] = FloatOpenDownload(
            "pt_resnet50v1.5_imagenet_224_224_0.4_4.9G_1.1_M2.4.zip",
            directory,
            "resnet50/float/resnet50_fp16.onnx",
        )

    return models
