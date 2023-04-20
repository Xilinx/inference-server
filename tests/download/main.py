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

import argparse
import glob
import importlib
import os
import shutil
import sys
import urllib.error
import urllib.request
from pathlib import Path

sys.path.append(os.path.dirname(__file__))

import models as Repository

repository_path = os.getenv("AMDINFER_ROOT")
if repository_path is None:
    repository_path = Path.cwd()
else:
    repository_path = Path(repository_path)

tmp_dir = Path.cwd() / "tmp_downloader"
artifact_dir = repository_path / "external/artifacts"
repository_metadata_dir = repository_path / "external/repository_metadata"
test_assets_dir = repository_path / "tests/assets"


def download_http(model):
    filepath = str(tmp_dir / model.filename)
    with open(filepath, "wb") as f:
        req = urllib.request.Request(model.url)
        # need to specify the user-agent for accessing some URLs
        req.add_header("User-Agent", "amdinfer")
        try:
            response = urllib.request.urlopen(req)
        except urllib.error.URLError as e:
            print(f"Could not download file: {e.reason}")
            return ""
        f.write(response.read())

    new_file = model.extract(filepath)
    os.remove(filepath)
    return new_file


def create_package(model_file: str, metadata: Path, repository_dir: Path):
    with open(metadata, "r") as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith("name:"):
                # assume name is of the form 'name: "name"'
                name = line.split(":")[1].strip().strip('"')
            if line.startswith("platform:"):
                platform = line.split(":")[1].strip().strip('"')

    if platform == "vitis_xmodel":
        extension = "xmodel"
    elif platform == "tensorflow_graphdef":
        extension = "pb"
    elif platform == "pytorch_torchscript":
        extension = "pt"
    elif platform == "onnx_onnxv1":
        extension = "onnx"
    else:
        print(
            f"Skipping making a repository package for {model_file} because it's using an unsupported platform: {platform}"
        )
        return

    print("  Creating repository package for this file")
    os.makedirs(repository_dir / name, exist_ok=True)
    shutil.copyfile(metadata, repository_dir / name / "config.pbtxt")
    os.makedirs(repository_dir / name / "1", exist_ok=True)
    shutil.copyfile(
        model_file, repository_dir / name / "1" / ("saved_model." + extension)
    )


def download(model, args: argparse.Namespace):
    # model.get_path_to_file() shouldn't be cached to a variable because the extraction
    # can overwrite the model's source_path which affects the name of the file.
    # this is used, for example, to deal with auto-converting .pth files to .pt
    print(
        f"Downloading {model.source_path} from {model.url} to {model.get_path_to_file()}"
    )
    if args.dry_run:
        return model.get_path_to_file()

    # always extract local files
    if model.url.startswith("file://"):
        model.extract(model.filename)
        return model.get_path_to_file()

    if model.get_path_to_file().exists():
        print(f"{model.get_path_to_file()} already exists, skipping download")
        return model.get_path_to_file()

    if model.url.startswith("http://") or model.url.startswith("https://"):
        download_http(model)
    else:
        raise NotImplementedError(f"Unknown URL protocol: {model.url}")

    return model.get_path_to_file()


def get_models(args):
    model_names = get_model_names()

    models = {}
    for model_name in model_names:
        if getattr(args, model_name):
            module = importlib.import_module("models." + model_name)
            models.update(module.get(args))

    return models


def strip_path_to_repository(absolute_path):
    absolute_path = str(absolute_path)
    if absolute_path.startswith(str(repository_path)):
        # strip the path prefix to the repository. +1 to include the trailing /
        return absolute_path[len(str(repository_path)) + 1 :]
    return absolute_path


def main(args: argparse.Namespace):
    args = update_args(args)

    if not args.dry_run:
        if not os.path.exists(tmp_dir):
            os.makedirs(tmp_dir)

        args.destination.mkdir(parents=True, exist_ok=True)
    else:
        print(f"Creating {str(tmp_dir)} if it doesn't exist")
        print(f"Creating {str(args.destination)} if it doesn't exist")

    models = get_models(args)

    repository_dir = args.destination / "repository"

    model_paths = {}
    for key, model in models.items():
        model.location = args.destination / model.location
        full_path: Path = download(model, args)

        metadata_file = (
            str(full_path).replace(f"{str(args.destination)}/", "").replace("/", "_")
        )
        metadata_file_path = repository_metadata_dir / (metadata_file + ".pbtxt")
        if metadata_file_path.exists():
            create_package(full_path, metadata_file_path, repository_dir)

        model_paths[key] = strip_path_to_repository(full_path)

    # add existing test assets to the assets list
    for f in os.listdir(str(test_assets_dir)):
        full_path = str(test_assets_dir / f)
        if os.path.isfile(full_path):
            model_paths[f"asset_{f}"] = strip_path_to_repository(full_path)

    artifact_data = repository_path / "tests/assets/artifacts.txt"
    if not args.dry_run:
        with open(artifact_data, "w+") as f:
            for key, path in model_paths.items():
                f.write(f"{key}:{path}\n")
        shutil.rmtree(tmp_dir)
    else:
        print(f"Saving artifact data to {str(artifact_data)}")
        print(model_paths)


def get_model_names():
    files = [
        os.path.splitext(os.path.basename(x))[0]
        for x in glob.iglob(os.path.dirname(Repository.__file__) + "/*.py")
    ]
    files.remove("__init__")
    return files


def get_parser(parser=None):
    top_level = parser is None
    if top_level:
        parser = argparse.ArgumentParser(
            prog="download",
            description="Download script for tests assets and models",
            add_help=False,
        )

    options = parser.add_argument_group("Options")
    options.add_argument(
        "--destination",
        action="store",
        default=artifact_dir,
        type=Path,
        help=f"path to save the downloaded files to. Defaults to {str(artifact_dir)}",
    )
    if top_level:
        options.add_argument(
            "--dry-run",
            action="store_true",
            help="print the actual commands that would be run without running them",
        )
    options.add_argument(
        "-h", "--help", action="help", help="show this help message and exit"
    )

    backends = parser.add_argument_group("Backends")
    backends.add_argument(
        "--all-backends", action="store_true", help="download files for all backends"
    )
    backends.add_argument(
        "--vitis", action="store_true", help="download all vitis-related files"
    )
    backends.add_argument(
        "--ptzendnn", action="store_true", help="download all ptzendnn-related files"
    )
    backends.add_argument(
        "--tfzendnn", action="store_true", help="download all tfzendnn-related files"
    )
    backends.add_argument(
        "--zendnn", action="store_true", help="alias for --ptzendnn --tfzendnn"
    )
    backends.add_argument(
        "--migraphx", action="store_true", help="download all migraphx-related files"
    )

    models = parser.add_argument_group("Models")
    models.add_argument(
        "--all-models", action="store_true", help="download files for all models"
    )
    model_names = get_model_names()
    for model_name in model_names:
        models.add_argument(
            "--" + model_name, action="store_true", help=f"download {model_name} files"
        )

    return parser


def update_args(args):
    """
    Parse the meta arguments and apply them as concrete values

    Args:
        args (argparse.namespace): the arguments from the command-line

    Returns:
        argparse.namespace: the updated arguments
    """
    if args.all_backends:
        args.vitis = True
        args.zendnn = True
        args.migraphx = True

    if args.zendnn:
        args.ptzendnn = True
        args.tfzendnn = True

    if args.all_models:
        model_names = get_model_names()
        for model_name in model_names:
            setattr(args, model_name, True)

    return args


if __name__ == "__main__":
    parser = get_parser()

    args = parser.parse_args()
    main(args)
