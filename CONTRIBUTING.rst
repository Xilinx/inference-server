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

Contributing
============

AMD Inference Server is in active development and many things need to be done so thanks for helping!


Ways to contribute
------------------

You can contribute in a variety of ways depending on your experience, time, and permissions.
The easiest way to help out is react and vote up issues and discussions that are important to you.

Idea generation
^^^^^^^^^^^^^^^

If you have ideas on new things that may be needed or changing something, `start a discussion <https://github.com/Xilinx/inference-server/discussions/new?category=ideas>`_.
New idea discussions don't need to be fully fleshed out or limited in scope.
Instead, use it as a notepad for ideas that you have so they can be remembered and tracked.
Having it as a discussion allows the community and maintainers to elaborate on the idea and determine if it's something of interest.

Raise issues
^^^^^^^^^^^^

An issue can be a bug report, a new feature request, or a change to documentation.
They're used to track concrete measurable tasks that need to get done, which means there should be a clear way to determine that the issue is resolved.
If you find a bug, give us as much information as you can about your environment, hardware, steps to reproduce and relevant logs.
If you can point to the code that's causing a problem, that helps a lot too!

Issues may be raised independently from ideas if the scope is already well-defined.
But ideas will also naturally result in new issues aimed at implementing the idea.
These related issues aimed at implementing this idea should link back to the original idea for context.
If an issue is better suited as a discussion before being formalized as an issue, it will be moved.

Triage
^^^^^^

Managing and organizing the repository are tasks for those with appropriate permissions:

* Issues that are better suited for discussions should be moved
* Issues should be labeled appropriately to categorize them and marked active once approved
* Projects should be used to track upcoming releases or deadlines
* Issues should be added to the appropriate project(s) if they're committed to for it

Raise pull requests
^^^^^^^^^^^^^^^^^^^

A pull request should be made against an active issue.
An active issue indicates that it has gone through a discussion already and is approved for development.
You can fork the repository, make your additions and raise a pull request.
Remember to :ref:`sign your git commits <Sign your work>`_.
The pull request will be reviewed by an owner and eventually approved for testing.
If the automated testing passes, then it can be merged in.
All code is licensed under the terms of the LICENSE file included in the repository.
Your contribution will be accepted under the same license.

Sign your work
""""""""""""""

Please use the *Signed-off-by* line at the end of your patch which indicates that you accept the `Developer Certificate of Origin (DCO) <https://developercertificate.org/>`_, reproduced below:

.. code-block:: text

    Developer Certificate of Origin
    Version 1.1

    Copyright (C) 2004, 2006 The Linux Foundation and its contributors.
    1 Letterman Drive
    Suite D4700
    San Francisco, CA, 94129

    Everyone is permitted to copy and distribute verbatim copies of this
    license document, but changing it is not allowed.


    Developer's Certificate of Origin 1.1

    By making a contribution to this project, I certify that:

    (a) The contribution was created in whole or in part by me and I
        have the right to submit it under the open source license
        indicated in the file; or

    (b) The contribution is based upon previous work that, to the best
        of my knowledge, is covered under an appropriate open source
        license and I have the right under that license to submit that
        work with modifications, whether created in whole or in part
        by me, under the same open source license (unless I am
        permitted to submit under a different license), as indicated
        in the file; or

    (c) The contribution was provided directly to me by some other
        person who certified (a), (b) or (c) and I have not modified
        it.

    (d) I understand and agree that this project and the contribution
        are public and that a record of the contribution (including all
        personal information I submit with it, including my sign-off) is
        maintained indefinitely and may be redistributed consistent with
        this project or the open source license(s) involved.


Here is an example commit message that indicates that the contributor accepts the DCO:

.. code-block:: text

    This is my commit message

    Signed-off-by: Jane Doe <jane.doe@example.com>

You can also add the sign-off statement automatically when committing with git:

.. code-block:: console

    $ git commit -s -m "This is my commit message"

Consider signing your commit with GPG as well.
You can see more information about commit signature verification on `Github <https://docs.github.com/en/authentication/managing-commit-signature-verification/signing-commits>`_.

Style guide
-----------

``pre-commit`` is used to enforce style and is included in the development container.
Install it with ``pre-commit install`` to configure the pre-commit hook.
Add tests to validate your changes.

Documentation
^^^^^^^^^^^^^

The documentation for the AMD Inference Server is written in reStructuredText and is located in the ``docs/`` directory.
If you are unfamiliar with reStructuredText, check out a `basic tutorial <https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html>`_

Headers
"""""""

Headers are denoted in reStructuredText with a series of punctuation characters at least as long as the title.
While it does not enforce that a particular character denotes a particular hierarchy, you should use the following convention that matches the `Python convention <https://devguide.python.org/documentation/markup/#sections>`_:

* ``#`` with overline, for parts
* ``*`` with overline, for chapters
* ``=``, for sections
* ``-``, for subsections
* ``^``, for subsubsections
* ``"``, for paragraphs

Admonitions
"""""""""""

Admonition boxes can be used to highlight and draw attention to points.
They should be used sparingly to avoid distracting the reader.
While reStructuredText supports many types of admonitions, the following groups of admonitions share the same coloring style in our theme.

* Blue: note, admonition
* Green: hint, important, tip
* Yellow: attention, caution, warning
* Red: danger, error

For maintaining visual consistency, admonitions in the same class should convey the same relative importance.

========== =====
Admonition Usage
========== =====
Blue       Contains useful non-essential information and does not suggest an action for the reader to take
Green      Contains useful information or suggests an action for the reader to take
Yellow     Contains important information or highlights unexpected side effects of events
Red        Contains critical information
========== =====
