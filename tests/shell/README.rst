..
    Copyright 2022 Xilinx Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Shell Script Testing
====================

We use `BATS <https://github.com/bats-core/bats-core>`__ to test shell scripts.

To use:

.. code-block:: bash

    # from the repository root
    mkdir -p ./tests/shell/test_helper
    cd ./tests/shell/test_helper
    git clone https://github.com/bats-core/bats-assert.git
    git clone https://github.com/bats-core/bats-support.git
    git clone https://github.com/bats-core/bats-file.git
    git clone https://github.com/bats-core/bats-core.git
    # can install bats or use the path to the executable
    sudo ./bats-core/install.sh /usr/local
    cd -
    bats ./tests/shell/
