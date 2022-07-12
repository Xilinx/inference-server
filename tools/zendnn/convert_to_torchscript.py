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

import os
import argparse


def main(args):
    """
    This method will read the eager PyTorch model,
    convert it to TorchScript model and save it.
    This will require PyTorch python API.

    Args:
        args (argparse): Parsed argument dictionary

    Raises:
        FileNotFoundError: If graph given in argument is not found
        ModuleNotFoundError: If PyTorch python API not found
    """

    if not os.path.exists(args.graph):
        raise FileNotFoundError(
            "Model not found in the location: {}."
            "Please Check the parameter.".format(args.graph)
        )
    try:
        import torch
    except ModuleNotFoundError:
        raise ModuleNotFoundError(
            "PyTorch not installed, please install"
            " PyTorch/refer the docs to use this script. "
        )

    # the way this script is called determines which import will succeed, so try
    # both
    try:
        from resnet50 import resnet50
    except ImportError:
        from .resnet50 import resnet50

    # Prepare filename to save the model
    # <model>.pth-><model>.pt
    filename = args.graph.split(".")[0]
    save_filename = filename + ".pt"

    # Load the model according to the architecture
    model = resnet50().cpu().eval()
    model.load_state_dict(torch.load(args.graph))

    # Run a jit trace on the model and save the model
    model = torch.jit.script(model)
    model.save(save_filename)

    print("Converted TorchScript model saved at: " + save_filename)


if __name__ == "__main__":

    # Get the arguments required from the user
    parser = argparse.ArgumentParser(
        description="Convert PyTorch eager models to TorchScript models"
    )
    parser.add_argument(
        "--graph", "-g", type=str, required=True, help="Full path to the input graph"
    )

    # Parse arguments and call main
    args = parser.parse_args()
    main(args)
