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

from main import test_assets_dir
from model import Archive, File


def get(_args: argparse.Namespace):
    models = {}
    models["asset_Physicsworks.ogv"] = File(
        "https://upload.wikimedia.org/wikipedia/commons/c/c4/Physicsworks.ogv",
        test_assets_dir,
        "",
    )
    models["asset_girl-1867092_640.jpg"] = File(
        "https://cdn.pixabay.com/photo/2016/11/29/03/35/girl-1867092_640.jpg",
        test_assets_dir,
        "",
    )
    models["asset_adas.webm"] = Archive(
        "https://www.xilinx.com/bin/public/openDownload?filename=vitis_ai_runtime_r1.3.0_image_video.tar.gz",
        test_assets_dir,
        "adas_detection/video/adas.webm",
    )

    return models
