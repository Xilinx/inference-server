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

Testing
=======

The AMD Inference Server uses `pytest <https://docs.pytest.org>`__ to drive both Python and C++ tests.
The tests must be run in the development container.
In the container, use:

.. code-block:: console

    $ amdinfer test

By default, this will run all Python and C++ tests while skipping the ones that require unsupported hardware without error.
You can use ``--help`` to see the full options for this command.
A few important ones are mentioned here:

1. ``--cpp``: by default, this is set to ``all``. You can set the value of this flag to ``skip`` or ``only`` to not run or only run C++ tests, respectively.
2. ``-k``: use this flag to filter tests by name. This is a `pytest option <https://docs.pytest.org/en/latest/example/markers.html#using-k-expr-to-select-tests-based-on-their-name>`__

.. tip::

    In addition to the flags that are listed when you use ``--help``, any unknown flags are passed directly to pytest.
    This is how the ``-k`` flag works above.
    Refer to the pytest documentation for the flags you can use to control pytest.

Add a new test
--------------

The easiest way to add a new test is to find an existing test that is close to what you want to test.
Then, you can copy it and adapt it for the new test.
The tests are in the `tests <https://github.com/Xilinx/inference-server/tree/main/tests>`__ directory in the repository.

Add assets
^^^^^^^^^^

The inference tests require using images and other files as input data.
They also require models for performing inference.
If you need other input data or models for a new test, you need to add them to the repository's download script so they can be downloaded with ``amdinfer get``.
The downloading script is in `tests/download <https://github.com/Xilinx/inference-server/tree/main/tests/download>`__ directory.
You need to append to or create a new file in the ``models`` directory.
A downloaded asset, such as an image or model, is defined by a:

1. Key: the key is a unique string that tests will use to get this asset
2. URL: the location where to download the asset from
3. Destination: the location to place the downloaded asset relative to where the downloaded files are being saved
4. Source path: for archived files, a source path is used to define where the actual file is. It can be an empty string otherwise.

These values are arguments to a class that defines how to download the file.
If your asset is similar to download to an existing asset, you can reuse the same class.
Otherwise, you may need to create your own class for downloading your asset.
You should also gate the download of your asset with the appropriate backend it is intended for.

The downloaded files are placed in a local directory in the inference server repository, which is fixed to ``external/artifacts``.
