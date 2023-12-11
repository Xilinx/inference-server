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

from model import File


def get(args: argparse.Namespace):
    directory = Path("yolov4")

    models = {}
    if args.migraphx:
        models["onnx_yolov4_anchors"] = File(
            "https://github.com/onnx/models/raw/5faef4c33eba0395177850e1e31c4a6a9e634c82/vision/object_detection_segmentation/yolov4/dependencies/yolov4_anchors.txt",
            directory,
            "",
        )
        models["onnx_yolov4"] = File(
            "https://github.com/onnx/models/raw/5faef4c33eba0395177850e1e31c4a6a9e634c82/vision/object_detection_segmentation/yolov4/model/yolov4.onnx",
            directory,
            "",
        )
        models["onnx_yolov4_coco"] = File(
            "https://github.com/onnx/models/raw/5faef4c33eba0395177850e1e31c4a6a9e634c82/vision/object_detection_segmentation/yolov4/dependencies/coco.names",
            directory,
            "",
        )

    return models
