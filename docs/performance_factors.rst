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

Performance Factors
===================

AMD Inference Server's performance can be maximized by appropriately controlling the factors described here.

Hardware
--------

The :program:`amdinfer-server` executable or any application that links to AMD Inference Server should be run on a server-grade machine with adequate CPUs/threads and RAM.
We suggest at least 32GB of RAM and 6 core/12 threads.
Other processes running on the server should be minimized.

Compile the right version
-------------------------

Enable compiler optimizations by building with the :option:`--release` flag.

.. code-block:: console

    $ amdinfer build --release

Parallelism
-----------

There are several areas in which changing the level of parallelism may improve performance.

REST threads
^^^^^^^^^^^^

The number of threads that Drogon uses to receive incoming REST requests is defined by the value of ``kDefaultDrogonThreads``, which by default is set to 16.
Depending on your hardware and demand, you may get better performance by changing this value.

Sending requests
^^^^^^^^^^^^^^^^

Making requests to AMD Inference Server efficiently allows for overlap and batching to take place, which improves throughput.

For REST requests, make asynchronous requests so sequential requests don't block each other.
There are a few ways to do this.
For benchmarking, use ``wrk`` or other HTTP benchmarking executables that ensure maximum throughput.
If making requests from Python, use ``aiohttp`` or similar packages to make asynchronous requests instead of the ``requests`` package.
AMD Inference Server's Python API provides :py:meth:`~amdinfer.HttpClient.modelInfer` for synchronous requests with the :py:class:`HttpClient <amdinfer.HttpClient>` class.

For C++ applications, the same principle holds.
Using multiple threads to enqueue and dequeue requests to AMD Inference Server allows for higher throughput.
One example of how to do this is in the following snippet:

.. code-block:: cpp

    #include <future>
    #include <string>
    #include <utility>
    #include <vector>

    #include <concurrentqueue/blockingconcurrentqueue.h>

    #include "amdinfer/amdinfer.hpp"

    using FutureQueue = moodycamel::BlockingConcurrentQueue<std::future<amdinfer::InferenceResponse>>;

    void enqueue(const int images, const std::string& workerName,
                 amdinfer::InferenceRequestInput request, FutureQueue& my_queue) {
        for (int i = 0; i < images; i++) {
            auto future = amdinfer::enqueue(workerName, request);
            my_queue.enqueue(std::move(future));
        }
    }

    void dequeue(int images, FutureQueue& my_queue) {
        std::future<amdinfer::InferenceResponse> element;
        for (auto i = 0; i < images; i++) {
            my_queue.wait_dequeue(element);
            auto results = element.get();
        }
    }

    int main(){
        const int threads = 4;
        const int images = 100;
        const std::string workerName = "Xmodel";

        ...

        std::vector<std::future<void>> futures;
        FutureQueue my_queue;

        futures.reserve(threads);
        for (int i = 0; i < threads; i++) {
            amdinfer::InferenceRequestInput request;
            std::thread{enqueue, images/threads, workerName, request, std::ref(my_queue)}.detach();
            futures.push_back(std::async(std::launch::async, dequeue, images / threads,
                                         std::ref(my_queue)));
        }

        ...

        for (auto& future : futures) {
            future.get();
        }

        ...

    }

Enqueuing and dequeueing in parallel improves performance because it minimizes the number of active requests at any given time.

Duplicating workers
^^^^^^^^^^^^^^^^^^^

Worker duplication is one method of parallelizing a worker.
By default, requesting to load a worker that has already been loaded does nothing.
However, workers can be manually duplicated for increased throughput.
All workers accept the ``share`` load-time parameter.
This parameter is assumed to be true if unspecified but it can be set to false to force AMD Inference Server to allocate a new worker.
Each of these workers will share a common batcher, which will push requests to a task queue for the workers in the group.

.. code-block:: python

    client = amdinfer.HttpClient("127.0.0.1:8998")

    parameters = {"share": False}

    # this will load three copies of this worker
    response = client.load("Resnet50", parameters)
    client.load("Resnet50", parameters)
    client.load("Resnet50", parameters)

For example, each Xmodel worker allocates a separate runner which is used to make requests to the FPGA.
Duplicating this worker may result in using more physical computing units (CUs) on the FPGA or requesting more CUs from other FPGAs on the host machine, if available.
However, consuming more CUs does not necessarily improve performance if data cannot be funneled to them fast enough.
Efficient use of these runners requires parallel request submissions.
The Xmodel worker supports this with the ``threads`` load-time parameter, which controls how many threads exist to push work to the runner.
Thus, you may need to load multiple Xmodel workers to allocate sufficient hardware on the machine and then further run each worker with multiple threads to push data to each CU for the best performance.

.. code-block:: python

    client = amdinfer.HttpClient("127.0.0.1:8998")

    parameters = {"threads": 5}

    response = client.load("Xmodel", parameters)

    # since there's no "share" parameter, this call will do nothing as it's value
    # is assumed true
    client.load("Xmodel", parameters)
