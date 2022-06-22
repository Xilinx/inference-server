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

REST Endpoints
==============

The REST endpoints are based on `KServe's v2 specification <https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md>`__.
Additional endpoints are driven by community adoption.

Health
------

*  GET ``v2/health/live``: Check if the server is live
*  GET ``v2/health/ready``: Check if the server is ready for inference requests
*  GET ``v2/models/{model}/ready``: Check if a particular model is ready for inference requests

Metadata
--------

*  GET ``v2``: Get AMD Inference Server's metadata
*  GET ``v2/hardware``: Get a string describing the number and type of kernels that are available
*  GET ``v2/models``: Get a list of active models
*  GET ``v2/models/{model}``: Get model metadata

Inference
---------

*  POST ``v2/repository/models/{model}/load``: Load a model
*  POST ``v2/workers/{worker}/load``: Load a worker. The HTML body in the response contains the endpoint to use for subsequent requests
*  POST ``v2/repository/models/{model}/unload``: Unload a model
*  POST ``v2/workers/{worker}/unload``: Unload a worker
*  POST ``v2/models/{model}/infer``: Make an inference request to a particular model

Observation
-----------

*  GET ``metrics``: Get Prometheus metrics
