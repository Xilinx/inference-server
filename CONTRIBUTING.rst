.. _contributing:

Contributing
============

AMD Inference Server is in active development and many things need to be done so thanks for helping!
You can contribute in a variety of ways.

Ways to Contribute
------------------

* Raise issues to report bugs! If you find a bug, give us as much information as you can about your environment, hardware, steps to reproduce and relevant logs. If you can point to the code that's causing a problem, that helps a lot too!
* Raise issues to suggest or request new features and changes to existing ones to improve functionality, performance and/or quality. Comment and vote up issues that you think should be prioritized.
* Submit pull requests against active issues. See `Contributing Code`_ for more information.

Contributing Code
------------------

The suggested model of contributing new code is to pick an issue (or raise a new one) and comment about wanting to tackle it.
This lets others know that there's active development on it. Then, you can fork this repository, make your additions and submit a pull request for merging.
When sending code sign your work as described below.
All code is licensed under the terms of the LICENSE file included in the repository.
Your contribution will be accepted under the same license.

Sign Your Work
^^^^^^^^^^^^^^

Please use the *Signed-off-by* line at the end of your patch which indicates that you accept the Developer Certificate of Origin (DCO) defined by https://developercertificate.org/ reproduced below:

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


Here is an example Signed-off-by line which indicates that the contributor accepts DCO:

.. code-block:: text

  This is my commit message

  Signed-off-by: Jane Doe <jane.doe@example.com>

Style Guide
-----------

``pre-commit`` is used to enforce style and is included in the development container.
Install it with ``pre-commit install`` to configure the pre-commit hook.
Add tests to validate your changes.
