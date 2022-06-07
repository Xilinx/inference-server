..
    Copyright 2021 Xilinx Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Changelog
=========

All notable changes to this project will be documented in this file.

The format is based on `Keep a Changelog <https://keepachangelog.com/en/1.0.0/>`__,
and this project adheres to `Semantic Versioning <https://semver.org/spec/v2.0.0.html>`__.

Unreleased
----------

Added
^^^^^

- HTTP/REST C++ client (:commit:`cbf33b8`)
- gRPC API based on KServe v2 API (:commit:`37a6aad` and others)
- TensorFlow/Pytorch + ZenDNN backend (:pr:`17` and :pr:`21`)
- 'ServerMetadata' endpoint to the API (:commit:`7747911`)
- 'modelList' endpoint to the API (:commit:`7477b7d`)
- Parse JSON data as string in HTTP body (:commit:`694800e`)

Changed
^^^^^^^

- Use Pybind11 to create Python API (:pr:`20`)
- Two logs are created now: server and client
- Logging macro is now ``PROTEUS_LOG_*``
- Loading workers is now case-insensitive (:commit:`14ed4ef`)

Fixed
^^^^^

- Get the right request size in the batcher when enqueuing with the C++ API (:commit:`d1ad81d`)
- Construct responses correctly in the XModel worker if there are multiple input buffers (:commit:`d1ad81d`)
- Populate the right number of offsets in the hard batcher (:commit:`6666142`)
- Calculate offset values correctly during batching (:commit:`8c7534b`)


:github:`0.1.0 <Xilinx/inference-server/releases/tag/v0.1.0>` - 2022-02-08
--------------------------------------------------------------------------

Added
^^^^^

- Core inference server functionality
- Batching support
- Support for running multiple workers simultaneously
- Support for different batcher and buffer implementations
- XModel support
- Logging, metrics and tracing support
- REST API based on KServe v2 API
- C++ API
- Python library for REST
- Documentation, examples, and some tests
- Experimental GUI
