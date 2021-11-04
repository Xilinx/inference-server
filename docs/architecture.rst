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

Architecture
============

.. image:: assets/architecture.png
    :alt: Diagram showing Xilinx Inference Server's architecture


HTTP Framework
--------------

We chose to use Drogon for our web framework for a few reasons:

* Based on the benchmarks at `TechEmpower <https://github.com/TechEmpower/FrameworkBenchmarks/>`__, Drogon is high-performing (unlike CppCMS and Treefrog)
* It is more stable and active than Lithium, another high-performing framework (Lithium is newer)
* Active on Github with versioned releases (unlike Pistache and Lithium)

.. _architectureWorkers:

Workers
-------

Workers are the smallest unit that Xilinx Inference Server manages.
A worker may be as simple or complex as you like: as long as it adheres to Xilinx Inference Server's interface.
Each worker is compiled as a shared object that Xilinx Inference Server can dynamically open.
This shared object should define an accessor method called ``getWorker()`` and a class that extends Xilinx Inference Server's Worker class.
Each worker comes with an input queue, which is how the worker receives incoming requests.

Xilinx Inference Server starts a worker by calling the ``getWorker()`` method that returns a new instance of the worker's class.

.. code-block:: c++

    extern "C" {
    proteus::workers::Worker* getWorker() { return new proteus::workers::MyWorkerClass(); }
    }

From this class, Xilinx Inference Server calls the following methods:

#. ``init()``: Perform low-cost one-time initialization for the worker
#. ``allocate()``: Allocate input and output buffers for the worker that are used to store incoming request data and computed results. The worker stores a vector of buffer pointers that constitute its buffer pool.
#. ``acquire()``: Reserve any hardware resources that may be needed as well as perform any remaining higher-cost initialization

After the these calls, Xilinx Inference Server calls the worker's ``spawn()`` which returns a new thread that runs the worker's ``run()`` method.
The ``run()`` method performs the body of the work for the worker.
In an infinite loop, it should read its input queue and wait for requests.
If the request is not a ``nullptr``, which signifies that Xilinx Inference Server is requesting the worker to shutdown, it should be parsed to extract the incoming data and the worker should operate on it.
Once the work is complete, the worker can reply back to the client.

On receiving a ``nullptr`` request, the worker should begin its shutdown sequence by exiting its loop to poll the input queue.
For well-behaved shutdown, Xilinx Inference Server will perform the following steps during shutdown:

#. ``release()``: Free any hardware resources and undo any initialization operations performed in ``acquire()`` if needed.
#. ``deallocate()``: Free the allocated buffers for the worker
#. ``destroy()``: Perform any final cleanup before the worker's thread is joined

External Processing
^^^^^^^^^^^^^^^^^^^

Workers, by virtue of their generic structure, may be highly complex and call entirely external applications for processing data.
Xilinx Inference Server supports this use case and suggests the following for organizing code:

* The external application can be brought in similarly to how existing external applications are brought in to Xilinx Inference Server already with Cmake
* The general worker structure should follow the existing model for native Xilinx Inference Server workers as defined above
* After determining that a request is valid, the worker should convert the native Xilinx Inference Server request into something that the external application understands
* Then, the data can be passed over to the external application. Any changes and updates to how the external application processes data should be made in its own repository.
* The external application should return its results back to the Xilinx Inference Server worker
* Convert the response back to Xilinx Inference Server's native format (if needed) and reply to the client

Currently, there are no rules that Xilinx Inference Server enforces for what workers are allowed to do and if they must expose any other functionality to Xilinx Inference Server though this will change in the future.
For example, Xilinx Inference Server will eventually need to send health check requests to workers that must be responded to appropriately.
