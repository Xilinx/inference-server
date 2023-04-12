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
import importlib.machinery
import importlib.util
import os
import shutil
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

sys.path.append(os.path.dirname(__file__))

import models as Repository

repository_path = os.getenv("AMDINFER_ROOT")
if repository_path is None:
    print("AMDINFER_ROOT not defined in the environment")
    sys.exit(1)

tmp_dir = Path.cwd() / "tmp_downloader"
artifact_dir = Path(repository_path) / "external/artifacts"
repository_metadata_dir = Path(repository_path) / "external/repository_metadata"
u250_dir = artifact_dir / "u200_u250"
tensorflow_dir = artifact_dir / "tensorflow"
pytorch_dir = artifact_dir / "pytorch"
onnx_dir = artifact_dir / "onnx"
repository_dir = artifact_dir / "repository"
test_assets_dir = Path(f"{repository_path}/tests/assets")


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


# def get_new_file(model, old_files, new_files):
#     # if there are no new files, try to see if we can get the downloaded file
#     if not new_files:
#         for file in old_files:
#             # for single downloaded files, the file itself will be in the list
#             if file.endswith(str(model.filename)):
#                 return file
#         raise ValueError(f"No new downloaded model could be found in {model.location}")
#     if len(new_files) == 1:
#         return new_files[0]
#     for file in new_files:
#         # check if file ends with any of the supported model file endings
#         if file.endswith(tuple(model.models)):
#             return file
#     raise ValueError(f"No new downloaded model could be found in {model.location}")


def create_package(model_file: str, metadata: Path):
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
    print(f"Downloading {model.source_path} from {model.url} to {model.location}")
    if args.dry_run:
        return model.location

    # if model.location.exists():
    #     old_files = list(glob.iglob(str(model.location / "**/*"), recursive=True))
    # else:
    #     old_files = []

    # new_file = None
    if model.get_path_to_file().exists():
        print(f"{model.get_path_to_file()} already exists, skipping download")
        return model.get_path_to_file()

    if model.url.startswith("file://"):
        model.extract(model.filename)
    elif model.url.startswith("http://") or model.url.startswith("https://"):
        download_http(model)
    else:
        raise NotImplementedError(f"Unknown URL protocol: {model.url}")

    # files = glob.iglob(str(model.location / "**/*"), recursive=True)
    # if new_file is not None:
    #     new_files = [new_file]
    # else:
    #     new_files = [file for file in files if file not in old_files]

    # new_file = get_new_file(model, old_files, new_files)

    new_file = model.get_path_to_file()

    metadata_file = str(new_file).replace(f"{str(artifact_dir)}/", "").replace("/", "_")
    metadata_file_path = repository_metadata_dir / (metadata_file + ".pbtxt")
    if metadata_file_path.exists():
        create_package(new_file, metadata_file_path)

    return new_file


# def downloader_one_file_from_archive(
#     archive_filepath: str, archive: Path, location: Path, source_path: str
# ):
#     downloader(archive_filepath, archive, tmp_dir)
#     filepath = Path(tmp_dir / source_path)
#     new_filepath = location / filepath.name
#     if filepath.exists() and not new_filepath.exists():
#         shutil.move(str(filepath), str(new_filepath))
#     return str(new_filepath)


# def downloader_adas(filepath: str, filename: Path, location: Path):
#     return downloader_one_file_from_archive(
#         filepath, filename, location, "adas_detection/video/adas.webm"
#     )


# def downloader_bert_vocab(filepath: str, filename: Path, location: Path):
#     return downloader_one_file_from_archive(
#         filepath, filename, location, "uncased_L-12_H-768_A-12/vocab.txt"
#     )


# def get_models(args: argparse.Namespace):
#     models = {
#         "asset_Physicsworks.ogv": File(
#             "https://upload.wikimedia.org/wikipedia/commons/c/c4/Physicsworks.ogv",
#             test_assets_dir,
#         ),
#         "asset_girl-1867092_640.jpg": File(
#             "https://cdn.pixabay.com/photo/2016/11/29/03/35/girl-1867092_640.jpg",
#             test_assets_dir,
#         ),
#         "asset_adas.webm": File(
#             "https://www.xilinx.com/bin/public/openDownload?filename=vitis_ai_runtime_r1.3.0_image_video.tar.gz",
#             test_assets_dir,
#             downloader_adas,
#         ),
#     }

#     if args.vitis:
#         models.update(
#             {
#                 "u250_densebox": XModelOpenDownload(
#                     "densebox_320_320-u200-u250-r2.5.0.tar.gz", u250_dir
#                 ),
#                 "u250_resnet50": XModelOpenDownload(
#                     "resnet_v1_50_tf-u200-u250-r2.5.0.tar.gz", u250_dir
#                 ),
#                 "u250_yolov3": XModelOpenDownload(
#                     "yolov3_voc_tf-u200-u250-r2.5.0.tar.gz", u250_dir
#                 ),
#             }
#         )

#     if args.ptzendnn:
#         models.update(
#             {
#                 "pt_resnet50": FloatOpenDownload(
#                     "pt_resnet50_imagenet_224_224_8.2G_2.5.zip", pytorch_dir
#                 ),
#             }
#         )

#     if args.tfzendnn:
#         models.update(
#             {
#                 "tf_resnet50": FloatOpenDownload(
#                     "tf_resnetv1_50_imagenet_224_224_6.97G_2.5.zip", tensorflow_dir
#                 ),
#                 "tf_mnist": LocalFile(
#                     str(test_assets_dir / "mnist.zip"), repository_dir
#                 ),
#             }
#         )

#     if args.migraphx:
#         models.update(
#             {
#                 "onnx_resnet50": File(
#                     "https://github.com/onnx/models/raw/main/vision/classification/resnet/model/resnet50-v2-7.onnx",
#                     onnx_dir / "resnet50v2",
#                 ),
#                 "onnx_resnet50_val": File(
#                     "https://github.com/mvermeulen/rocm-migraphx/raw/master/datasets/imagenet/val.txt",
#                     onnx_dir / "resnet50v2",
#                 ),
#                 "onnx_yolov4_anchors": File(
#                     "https://github.com/onnx/models/raw/main/vision/object_detection_segmentation/yolov4/dependencies/yolov4_anchors.txt",
#                     onnx_dir / "yolov4",
#                 ),
#                 "onnx_yolov4": File(
#                     "https://github.com/onnx/models/raw/main/vision/object_detection_segmentation/yolov4/model/yolov4.onnx",
#                     onnx_dir / "yolov4",
#                 ),
#                 "onnx_yolov4_coco": File(
#                     "https://github.com/onnx/models/raw/main/vision/object_detection_segmentation/yolov4/dependencies/coco.names",
#                     onnx_dir / "yolov4",
#                 ),
#                 "onnx_bert": File(
#                     "https://github.com/onnx/models/raw/main/text/machine_comprehension/bert-squad/model/bertsquad-10.onnx",
#                     onnx_dir / "bert",
#                 ),
#                 # "onnx_resnet50_val": File(
#                 #     "https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/raw/develop/examples/nlp/python_bert_squad/requirements_bertsquad.txt",
#                 #     onnx_dir / "bert",
#                 # ),
#                 "onnx_bert_run": File(
#                     "https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/raw/develop/examples/nlp/python_bert_squad/run_onnx_squad.py",
#                     onnx_dir / "bert",
#                 ),
#                 "onnx_bert_input": File(
#                     "https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/raw/develop/examples/nlp/python_bert_squad/inputs_amd.json",
#                     onnx_dir / "bert",
#                 ),
#                 # "onnx_resnet50_val": File(
#                 #     "https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/raw/develop/examples/nlp/python_bert_squad/inputs.json",
#                 #     onnx_dir / "bert",
#                 # ),
#                 "onnx_bert_vocab": File(
#                     "https://storage.googleapis.com/bert_models/2018_10_18/uncased_L-12_H-768_A-12.zip",
#                     onnx_dir / "bert",
#                     downloader_bert_vocab,
#                 ),
#             }
#         )

#     return models


def get_models(args):
    model_names = get_model_names()

    models = {}
    for model_name in model_names:
        if getattr(args, model_name):
            # load a specific python file without consideration about modules/
            # packages
            # loader = importlib.machinery.SourceFileLoader(
            #     "custom_backends", args.custom_backends
            # )
            # print(Path(__file__).parent / "models" )
            # spec = importlib.util.spec_from_file_location(model_name, str(Path(__file__).parent / "models" / model_name) + ".py" )
            # module = importlib.util.module_from_spec(spec)
            # spec.loader.exec_module(module)
            module = importlib.import_module("models." + model_name)
            models.update(module.get(args))

    return models


def strip_path_to_repository(absolute_path):
    absolute_path = str(absolute_path)
    if absolute_path.startswith(repository_path):
        # strip the path prefix to the repository. +1 to include the trailing /
        return absolute_path[len(repository_path) + 1 :]
    return absolute_path


def main(args: argparse.Namespace):
    args = update_args(args)
    print(args)

    if not args.dry_run:
        if not os.path.exists(tmp_dir):
            os.makedirs(tmp_dir)

        # if artifact_dir.exists():
        #     shutil.rmtree(artifact_dir)
        args.destination.mkdir(parents=True, exist_ok=True)
        # os.makedirs(artifact_dir)
    else:
        print(f"Creating {str(tmp_dir)} if it doesn't exist")
        print(f"Creating {str(artifact_dir)} if it doesn't exist")

    models = get_models(args)

    model_paths = {}
    for key, model in models.items():
        model.location = args.destination / model.location
        full_path = download(model, args)
        model_paths[key] = strip_path_to_repository(full_path)

    # add existing test assets to the assets list
    for f in os.listdir(str(test_assets_dir)):
        full_path = str(test_assets_dir / f)
        if os.path.isfile(full_path):
            model_paths[f"asset_{f}"] = strip_path_to_repository(full_path)

    artifact_data = args.destination / "artifacts.txt"
    if not args.dry_run:
        with open(artifact_data, "w+") as f:
            for key, path in model_paths.items():
                f.write(f"{key}:{path}\n")
        shutil.rmtree(tmp_dir)
    else:
        print(f"Saving artifact data to {str(artifact_data)}:")
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
        help="path to save the downloaded files to",
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

    platforms = parser.add_argument_group("Platforms")
    platforms.add_argument(
        "--all-platforms", action="store_true", help="download files for all platforms"
    )
    platforms.add_argument(
        "--vitis", action="store_true", help="download all vitis-related files"
    )
    platforms.add_argument(
        "--ptzendnn", action="store_true", help="download all ptzendnn-related files"
    )
    platforms.add_argument(
        "--tfzendnn", action="store_true", help="download all tfzendnn-related files"
    )
    platforms.add_argument(
        "--zendnn", action="store_true", help="download all zendnn-related files"
    )
    platforms.add_argument(
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
    if args.all_platforms:
        args.vitis = True
        args.zendnn = True
        args.migraphx = True

    if args.zendnn:
        args.ptzendnn = True
        args.tfzendnn = True

    if args.all_models:
        model_names = get_model_names()
        print(model_names)
        for model_name in model_names:
            setattr(args, model_name, True)

    return args


if __name__ == "__main__":
    parser = get_parser()

    args = parser.parse_args()
    main(args)
