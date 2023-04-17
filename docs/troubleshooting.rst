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

Troubleshooting
===============

If things don't work as expected, use this page to help guide your debugging.
The more information you can gather, the easier it will be for you to fix the problem yourself or provide detailed information in a bug report.

Use server logs
---------------

You will need access to the running server to debug most issues.
While the server may respond to erroneous requests from the client with information about what failed, it will probably not have sufficient context to learn the underlying cause.
Instead, you should look at the server logs, if they are enabled.
By default, the server logs are in ``~/.amdinfer/logs/server.log``.
The home directory where the logs are located depend on how the server was started.
If it is running as root, as is the case in deployment containers, the root user's home directory is in ``/root/*``.
If it is running as the non-root user in the container, the home directory is in ``/home/amdinfer-user/*``.
