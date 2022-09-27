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
import os
import shutil
import subprocess
import sys
import tarfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path


def get_filename_from_url(url):
    if "www.xilinx.com/bin/public/openDownload" in url:
        search_str = "filename="
        return Path(url[url.index(search_str) + len(search_str) :])
    else:
        return Path(os.path.basename(urllib.parse.urlparse(url).path))


def downloader(filepath: str, filename: Path, location: Path):
    if not os.path.exists(location):
        os.makedirs(location)
    if str(filename).endswith("tar.gz"):
        with tarfile.open(filepath, mode="r:gz") as tar_file:
            tar_file.extractall(location)
    elif str(filename).endswith("zip"):
        with zipfile.ZipFile(filepath, "r") as zip_file:
            zip_file.extractall(location)
    else:
        shutil.copy2(filepath, str(location / filename))


class Model:
    # file endings used to find supported models
    models = ["pt", "pb", "xmodel", "onnx"]

    def __init__(self, url: str, location: Path, download_func=None):
        self.url = url
        self.location = location
        self.filename = get_filename_from_url(url)
        self.downloader = downloader if download_func is None else download_func

    def download(self, filepath: str, filename: Path, location: Path):
        self.downloader(filepath, filename, location)


class File(Model):
    models = []


tmp_dir = Path("/tmp/proteus_get")
artifact_dir = Path.cwd() / "external/artifacts"
u250_dir = artifact_dir / "u200_u250"
tensorflow_dir = artifact_dir / "tensorflow"
pytorch_dir = artifact_dir / "pytorch"
onnx_dir = artifact_dir / "onnx"
repository_dir = artifact_dir / "repository"
test_assets_dir = Path.cwd() / "tests/assets"


class XModelOpenDownload(Model):
    models = ["xmodel"]

    def __init__(self, filename, location):
        url = f"https://www.xilinx.com/bin/public/openDownload?filename={filename}"
        super().__init__(url, location)
        self.filename = Path(filename)


class FloatOpenDownload(XModelOpenDownload):
    models = ["pt", "pb"]

    def __init__(self, filename, location):
        super().__init__(filename, location)
        self.downloader = self.download

    def download(self, filepath: str, filename: Path, location: Path):
        downloader(filepath, filename, tmp_dir)
        model_path = str(tmp_dir / filename.with_suffix("") / "float/")
        for filename in os.listdir(model_path):
            if filename.endswith("pb") or filename.endswith("pth"):
                model_name = filename
                break
        destination = os.path.join(location, model_name)
        if not os.path.exists(location):
            os.makedirs(location)
        shutil.move(os.path.join(model_path, model_name), destination)
        if model_name.endswith("pth"):
            subprocess.check_call(
                [
                    sys.executable,
                    "-m",
                    "pip",
                    "install",
                    "-r",
                    "tools/zendnn/requirements.txt",
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.STDOUT,
            )
            import tools.zendnn.convert_to_torchscript as converter

            converter_args = argparse.Namespace
            converter_args.graph = destination
            converter.main(converter_args)


class LocalFile(Model):
    def __init__(self, filename, location):
        url = f"file://{filename}"
        super().__init__(url, location)


def download_http(model: Model):
    filepath = str(tmp_dir / model.filename)
    with open(filepath, "wb") as f:
        req = urllib.request.Request(model.url)
        # need to specify the user-agent for accessing some URLs
        req.add_header("User-Agent", "proteus")
        try:
            response = urllib.request.urlopen(req)
        except urllib.error.URLError as e:
            print(f"Could not download file: {e.reason}")
            return ""
        f.write(response.read())

    new_file = model.downloader(filepath, model.filename, model.location)
    os.remove(filepath)
    return new_file


def download(model: Model):
    print(f"Downloading {model.url} to {model.location}")
    if model.location.exists():
        old_files = list(glob.iglob(str(model.location / "**/*"), recursive=True))
    else:
        old_files = []

    new_file = None
    if model.url.startswith("file://"):
        # strip the file://
        filepath = model.url[7:]
        model.downloader(filepath, model.filename, model.location)
    elif model.url.startswith("http://") or model.url.startswith("https://"):
        new_file = download_http(model)
    else:
        raise NotImplementedError(f"Unknown URL protocol: {model.url}")

    files = glob.iglob(str(model.location / "**/*"), recursive=True)
    if new_file is not None:
        new_files = [new_file]
    else:
        new_files = [file for file in files if file not in old_files]

    # if there are no new files, try to see if we can get the downloaded file
    if not new_files:
        for file in old_files:
            # for single downloaded files, the file itself will be in the list
            if file.endswith(str(model.filename)):
                return file
        raise ValueError(f"No new downloaded model could be found in {model.location}")
    if len(new_files) == 1:
        return new_files[0]
    for file in new_files:
        if file.endswith(tuple(model.models)):
            return file
    raise ValueError(f"No new downloaded model could be found in {model.location}")


def downloader_adas(filepath: str, filename: Path, location: Path):
    downloader(filepath, filename, tmp_dir)
    adas_webm = Path(tmp_dir / "adas_detection/video/adas.webm")
    if adas_webm.exists() and not location.exists():
        shutil.move(str(adas_webm), str(location))
    return str(location / "adas.webm")


models = {
    "u250_densebox": XModelOpenDownload(
        "densebox_320_320-u200-u250-r2.5.0.tar.gz", u250_dir
    ),
    "u250_resnet50": XModelOpenDownload(
        "resnet_v1_50_tf-u200-u250-r2.5.0.tar.gz", u250_dir
    ),
    "u250_yolov3": XModelOpenDownload(
        "yolov3_voc_tf-u200-u250-r2.5.0.tar.gz", u250_dir
    ),
    "pt_resnet50": FloatOpenDownload(
        "pt_resnet50_imagenet_224_224_8.2G_2.5.zip", pytorch_dir
    ),
    "tf_resnet50": FloatOpenDownload(
        "tf_resnetv1_50_imagenet_224_224_6.97G_2.5.zip", tensorflow_dir
    ),
    "onnx_resnet50": File(
        "https://github.com/onnx/models/raw/main/vision/classification/resnet/model/resnet50-v2-7.onnx",
        onnx_dir / "resnet50v2",
    ),
    "onnx_resnet50_val": File(
        "https://github.com/mvermeulen/rocm-migraphx/raw/master/datasets/imagenet/val.txt",
        onnx_dir / "resnet50v2",
    ),
    "tf_mnist": LocalFile(str(test_assets_dir / "mnist.zip"), repository_dir),
    "asset_Physicsworks.ogv": File(
        "https://upload.wikimedia.org/wikipedia/commons/c/c4/Physicsworks.ogv",
        test_assets_dir,
    ),
    "asset_girl-1867092_640.jpg": File(
        "https://cdn.pixabay.com/photo/2016/11/29/03/35/girl-1867092_640.jpg",
        test_assets_dir,
    ),
    "asset_adas.webm": File(
        "https://www.xilinx.com/bin/public/openDownload?filename=vitis_ai_runtime_r1.3.0_image_video.tar.gz",
        test_assets_dir,
        downloader_adas,
    ),
}


def main():
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir)

    if artifact_dir.exists():
        shutil.rmtree(artifact_dir)
    os.makedirs(artifact_dir)

    model_paths = {}
    for key, model in models.items():
        model_paths[key] = download(model)

    # add existing test assets to the assets list
    for f in os.listdir(str(test_assets_dir)):
        full_path = str(test_assets_dir / f)
        if os.path.isfile(full_path):
            model_paths[f"asset_{f}"] = full_path

    with open(artifact_dir / "artifacts.txt", "w+") as f:
        for key, path in model_paths.items():
            f.write(f"{key}:{path}\n")

    shutil.rmtree(tmp_dir)


if __name__ == "__main__":
    main()
