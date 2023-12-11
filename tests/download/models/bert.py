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

from model import Archive, File


def get(args: argparse.Namespace):
    directory = Path("bert")

    models = {}
    if args.migraphx:
        models["onnx_bert"] = File(
            "https://github.com/onnx/models/raw/5faef4c33eba0395177850e1e31c4a6a9e634c82/text/machine_comprehension/bert-squad/model/bertsquad-10.onnx",
            directory,
            "",
        )
        models["onnx_bert_run"] = File(
            "https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/raw/develop/examples/nlp/python_bert_squad/run_onnx_squad.py",
            directory,
            "",
        )
        models["onnx_bert_input"] = File(
            "https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/raw/develop/examples/nlp/python_bert_squad/inputs_amd.json",
            directory,
            "",
        )
        models["onnx_bert_vocab"] = Archive(
            "https://storage.googleapis.com/bert_models/2018_10_18/uncased_L-12_H-768_A-12.zip",
            directory,
            "uncased_L-12_H-768_A-12/vocab.txt",
        )

    return models
