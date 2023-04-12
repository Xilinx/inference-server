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
import os
import shutil
import subprocess
import sys
import tarfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

from main import tmp_dir


def _get_filename_from_url(url: str) -> str:
    """
    Given a URL, extract the filename from it

    Args:
        url (str): URL to extract filename from

    Returns:
        str: filename
    """
    if "www.xilinx.com/bin/public/openDownload" in url:
        search_str = "filename="
        return url[url.index(search_str) + len(search_str) :]
    if url.startswith("file://"):
        # strip the file://
        return url[7:]
    return os.path.basename(urllib.parse.urlparse(url).path)


def extract(filepath: Path, destination: Path, _source_path=None):
    """
    Given a path to a file (regular or an archive), copy it to another location.
    If the path is an archive format, it is extracted to the location. If the
    destination location doesn't exist, it is created. Source path is not used
    but kept to keep the function signature

    Args:
        filepath (str): Path to the file to copy
        location (Path): Path to place the copied file(s)
        source_path (Path): Unused
    """
    if not isinstance(filepath, Path):
        filepath = Path(filepath)

    destination.mkdir(parents=True, exist_ok=True)
    filename = filepath.name
    if str(filename).endswith("tar.gz"):
        with tarfile.open(filepath, mode="r:gz") as tar_file:
            tar_file.extractall(destination)
    elif str(filename).endswith("zip"):
        with zipfile.ZipFile(filepath, "r") as zip_file:
            zip_file.extractall(destination)
    else:
        shutil.copy2(filepath, str(destination))


def extract_one_file(archive_path: Path, destination: Path, source_path: Path):
    extract(archive_path, tmp_dir)

    destination.mkdir(parents=True, exist_ok=True)
    filepath = tmp_dir / source_path
    new_filepath = destination / source_path.name
    if filepath.exists() and not new_filepath.exists():
        shutil.move(str(filepath), str(new_filepath))
    else:
        print(f"{str(filepath)} doesn't exist or {str(new_filepath)} already exists")


class Model:
    def __init__(self, url: str, location: Path, source_path: Path, extractor=extract):
        self.url = url
        self.location = location
        self.filename = _get_filename_from_url(url)
        self.source_path = Path(source_path)

        self._extract = extractor

    def extract(self, filepath: Path):
        self._extract(filepath, self.location, self.source_path)

    def get_path_to_file(self):
        # an empty string turns into a Path(".")
        if str(self.source_path) != ".":
            return self.location / self.source_path.name
        return self.location / self.filename


class File(Model):
    pass


class Archive(Model):
    def __init__(self, url: str, location: Path, source_path: Path):
        super().__init__(url, location, source_path, extract_one_file)


class XModelOpenDownload(Model):
    def __init__(self, filename: str, location: Path, source_path: Path):
        """
        Create an XModel downloaded from Xilinx Open Download

        Args:
            filename (str): name of the file to download
            location (Path): relative path to place the extracted model file
        """
        url = f"https://www.xilinx.com/bin/public/openDownload?filename={filename}"
        super().__init__(url, location, source_path, extract_one_file)
        self.filename = Path(filename)


class FloatOpenDownload(XModelOpenDownload):
    def __init__(self, filename: str, location: Path, source_path: Path):
        super().__init__(filename, location, source_path)

    def extract(self, filepath):
        super().extract(filepath)
        if str(self.get_path_to_file()).endswith("pth"):
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

            destination = self.get_path_to_file()

            converter_args = argparse.Namespace
            converter_args.graph = str(destination)
            converter.main(converter_args)

            self.source_path = self.source_path.stem + ".pt"


class LocalFile(Model):
    def __init__(self, filename: str, location: Path, source_path: Path):
        url = f"file://{filename}"
        super().__init__(url, location, source_path)
