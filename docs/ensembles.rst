..
    Copyright 2023 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Ensembles
=========

Ensembles are |define_ensemble|.
You can use ensembles to define a pipeline like pre-processing, inference, and post-processing on the server.

.. note::

    Ensembles are currently limited to :term:`chains <Chain>`.
    They must also be defined at load-time rather than at run-time.

Defining ensembles
------------------

Ensembles can be defined in two ways: in the model repository and with the API.

Model repository
^^^^^^^^^^^^^^^^

Ensembles can be defined in the model repository to enable their use with Docker and KServe deployments.
You can also use this approach if you want to load a model from a local repository using ``modelLoad``.
At a high-level, you must define the ensemble in the model's configuration file, place all the model files in the directory and the ensemble gets loaded as any other single model.
For more details, see :ref:`how to define ensembles in the model repository <model_repository:Ensembles>`.

API
^^^

You can also load a chain as a set of workers directly using the client API method ``loadEnsemble()`` and providing it an array of workers and corresponding parameters.
The method returns an array of endpoints, where the first endpoint corresponds to the endpoint used to send requests to the ensemble.
To unload an ensemble, you can use the client API method ``unloadModels()`` and provide the array of endpoints.
