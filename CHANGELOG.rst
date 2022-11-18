..
    Copyright 2021 Xilinx, Inc.
    Copyright 2022 Advanced Micro Devices, Inc.

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

- Allow building Debian package (:commit:`930fab2`)
- Add ``modelInferAsync`` to the API (:commit:`2f4a6c2`)
- Add ``inferAsyncOrdered`` as a client operator for making inferences in parallel (:pr:`66`)
- Support building Python wheels with cibuildwheel (:pr:`71`)
- Support XModels with multiple output tensors (:pr:`74`)
- Add FP16 support (:pr:`76`)
- Add more documentation (:pr:`85`, :pr:`90`)
- Add Python bindings for gRPC and Native clients (:pr:`88`)
- Add tests with KServe (:pr:`90`)

Changed
^^^^^^^

- Refactor pre- and post-processing functions in C++ (:commit:`42cf748`)
- Templatize Dockerfile for different base images (:pr:`71`)
- Use multiple HTTP clients internally for parallel HTTP requests (:pr:`66`)
- Update test asset downloading (:pr:`81`)
- Reimplement and align examples across platforms (:pr:`85`)
- Reorganize Python library (:pr:`88`)

Fixed
^^^^^

- Use input tensors in requests correctly (:pr:`61`)
- Fix bug with multiple input tensors (:pr:`74`)
- Align gRPC responses using non-gRPC-native data types with other input protocols (:pr:`81`)
- Fix the Manager's destructor (:pr:`88`)
- Fix using ``--no-user-config`` with ``proteus run`` (:pr:`89`)

:github:`0.2.0 <Xilinx/inference-server/releases/tag/v0.2.0>` - 2022-08-05
--------------------------------------------------------------------------

Added
^^^^^

- HTTP/REST C++ client (:commit:`cbf33b8`)
- gRPC API based on KServe v2 API (:commit:`37a6aad` and others)
- TensorFlow/Pytorch + ZenDNN backend (:pr:`17` and :pr:`21`)
- 'ServerMetadata' endpoint to the API (:commit:`7747911`)
- 'modelList' endpoint to the API (:commit:`7477b7d`)
- Parse JSON data as string in HTTP body (:commit:`694800e`)
- Directory monitoring for model loading (:commit:`6459797`)
- 'ModelMetadata' endpoint to the API (:commit:`22b9d1a`)
- MIGraphX backend (:pr:`34`)
- Pre-commit for style verification(:commit:`048bdd7`)

Changed
^^^^^^^

- Use Pybind11 to create Python API (:pr:`20`)
- Two logs are created now: server and client
- Logging macro is now ``PROTEUS_LOG_*``
- Loading workers is now case-insensitive (:commit:`14ed4ef` and :commit:`90a51ae`)
- Build AKS from source (:commit:`e04890f`)
- Use consistent custom exceptions (:issue:`30`)
- Update Docker build commands to opt-in to all backends (:pr:`43`)
- Renamed 'modelLoad' to 'workerLoad' and changed the behavior for 'modelLoad' (:pr:`27`)

Fixed
^^^^^

- Get the right request size in the batcher when enqueuing with the C++ API (:commit:`d1ad81d`)
- Construct responses correctly in the XModel worker if there are multiple input buffers (:commit:`d1ad81d`)
- Populate the right number of offsets in the hard batcher (:commit:`6666142`)
- Calculate offset values correctly during batching (:commit:`8c7534b`)
- Get correct library dependencies for production container (:commit:`14ed4ef`)
- Correctly throw an exception if a worker gets an error during initialization (:pr:`29`)
- Detect errors in HTTP client during loading (:commit:`99ffc33`)
- Construct batches with the right sizes (:pr:`57`)


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
