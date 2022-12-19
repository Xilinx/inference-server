..
    Copyright 2021 Xilinx, Inc.
    Copyright 2022, Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

REST Endpoints
==============

The REST endpoints are based on `KServe's v2 specification <https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md>`__.
Additional endpoints are driven by community adoption.
The :amdinferblob:`full OpenAPI 3.0 spec <docs/rest_api.yaml>` is available in the repository.

.. openapi:httpdomain:: rest_api.yaml
    :generate-examples-from-schemas:
