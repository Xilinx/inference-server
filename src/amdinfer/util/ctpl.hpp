// Copyright 2014 Vitaliy Vitsentiy
// Copyright 2021 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GUARD_AMDINFER_UTIL_CTPL
#define GUARD_AMDINFER_UTIL_CTPL

#include <concurrentqueue/concurrentqueue.h>

#include <atomic>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "amdinfer/util/thread.hpp"

// thread pool to run user's functors with signature
//      ret func(int id, other_params)
// where id is the index of the thread that runs the functor
// ret is some return type

namespace amdinfer::util {

/**
 * @brief The thread pool is configured with a number of threads and accepts
 * lambdas as functions to run in one of the threads in the pool.
 *
 */
class thread_pool {
 public:
  thread_pool();
  explicit thread_pool(int nThreads);
  thread_pool(int nThreads, int queueSize);

  thread_pool(const thread_pool &) = delete;
  thread_pool(thread_pool &&) = delete;
  thread_pool &operator=(const thread_pool &) = delete;
  thread_pool &operator=(thread_pool &&) = delete;

  // the destructor waits for all the functions in the queue to be finished
  ~thread_pool();

  // get the number of running threads in the pool
  int getSize() const;

  // number of idle threads
  int getIdle() const;
  std::thread &getThread(int i);

  // change the number of threads in the pool
  // should be called from one thread, otherwise be careful to not interleave,
  // also with this->stop() nThreads must be >= 0
  void resize(int thread_num);

  // empty the queue
  void clearQueue();

  // pops a functional wrapper to the original function
  std::function<void(int)> pop();

  // wait for all computing threads to finish and stop all threads
  // may be called asynchronously to not pause the calling thread while waiting
  // if wait == true, all the functions in the queue are run, otherwise the
  // queue is cleared without running the functions
  void stop(bool wait = false);

  template <typename F, typename... Rest>
  auto push(F &&f, Rest &&...rest) -> std::future<decltype(f(0, rest...))> {
    auto pck =
      std::make_shared<std::packaged_task<decltype(f(0, rest...))(int)>>(
        std::bind(std::forward<F>(f), std::placeholders::_1,
                  std::forward<Rest>(rest)...));

    auto *_f = new std::function<void(int id)>([pck](int id) { (*pck)(id); });
    q_.enqueue(_f);

    std::unique_lock lock(mutex_);
    cv_.notify_one();

    return pck->get_future();
  }

  // run the user's function that excepts argument int - id of the running
  // thread. returned value is templatized operator returns std::future, where
  // the user can get the result and rethrow the caught exceptions
  template <typename F>
  auto push(F &&f) -> std::future<decltype(f(0))> {
    auto pck = std::make_shared<std::packaged_task<decltype(f(0))(int)>>(
      std::forward<F>(f));

    auto *_f = new std::function<void(int id)>([pck](int id) { (*pck)(id); });
    q_.enqueue(_f);

    std::unique_lock lock(mutex_);
    cv_.notify_one();

    return pck->get_future();
  }

 private:
  void setThread(int i);

  std::vector<std::unique_ptr<std::thread>> threads_;
  std::vector<std::shared_ptr<std::atomic<bool>>> flags_;
  mutable moodycamel::ConcurrentQueue<std::function<void(int id)> *> q_;
  std::atomic<bool> done_ = false;
  std::atomic<bool> stop_ = false;
  std::atomic<int> waiting_ = 0;  // how many threads are waiting

  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace amdinfer::util

#endif  // GUARD_AMDINFER_UTIL_CTPL
