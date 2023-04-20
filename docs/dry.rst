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

..
    This page has blocks of text that are inserted in multiple pages in an
    effort to avoid duplicating text and making the documentation easier to
    maintain. The format for text snippets is:
        +<label>
        <contents>
        -<label>
    As an arbitrary convention, I'm using +/- prefixes to denote start and stop
    labels. Note that due to how including sections of pages works in RST, the
    labels must be entirely unique i.e. if you use "foobar" as a label, you
    cannot use "foobar" at the start of any other label. Therefore, prefer
    verbose labels to prevent unintentional matching.

+loading_the_backend_intro
There are multiple ways to load this backend to make it available for inference requests from clients.
If you are using a client's ``workerLoad()`` method:
-loading_the_backend_intro


+loading_the_backend_modelLoad
With a client's ``modelLoad()`` method or using the repository approach, you need to create a :ref:`model repository <model_repository:Model Repository>` and put a model in it.
To use this backend with your model, use |platform| as the platform for your model.

Then, you can load the model from the server after setting up the path to the model repository.
The server may be set to automatically load all models from the configured model repository or you can load it manually using ``modelLoad()``.
In this case, the endpoint is defined in the model's configuration file in the repository and it is used as the argument to ``modelLoad()``.

.. tabs::

    .. code-tab:: c++ C++

        // amdinfer::Client* client;
        // amdinfer::ParameterMap parameters;
        client->modelLoad(<model>, parameters)

    .. code-tab:: python Python

        # client = amdinfer.Client()
        # parameters = amdinfer.ParameterMap()
        client.modelLoad(<model>, parameters)

-loading_the_backend_modelLoad

+docker_pull_deployment_images
.. tabs::

    .. code-tab:: console CPU

        $ docker pull amdih/serve:uif1.1_zendnn_amdinfer_0.3.0

    .. code-tab:: text GPU

        $ docker pull amdih/serve:uif1.1_migraphx_amdinfer_0.3.0

    .. code-tab:: console FPGA

        # this image is not currently pre-built but you can build it yourself
-docker_pull_deployment_images
