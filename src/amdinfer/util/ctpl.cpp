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

#include "amdinfer/util/ctpl.hpp"

namespace amdinfer::util {

constexpr auto kThreadPoolLength = 100;

thread_pool::thread_pool() : thread_pool(0, kThreadPoolLength) {}

thread_pool::thread_pool(int thread_num)
  : thread_pool(thread_num, kThreadPoolLength) {}

thread_pool::thread_pool(int thread_num, int queue_size) : q_(queue_size) {
  if (thread_num > 0) {
    this->resize(thread_num);
  }
}

// the destructor waits for all the functions in the queue to be finished
thread_pool::~thread_pool() { this->stop(true); }

// get the number of running threads in the pool
int thread_pool::getSize() const { return static_cast<int>(threads_.size()); }

// number of idle threads
int thread_pool::getIdle() const { return waiting_; }

std::thread &thread_pool::getThread(int i) { return *threads_[i]; }

// change the number of threads in the pool
// should be called from one thread, otherwise be careful to not interleave,
// also with this->stop() nThreads must be >= 0
void thread_pool::resize(int thread_num) {
  if (!stop_ && !done_) {
    auto old_thread_num = static_cast<int>(threads_.size());
    if (old_thread_num <=
        thread_num) {  // if the number of threads is increased
      threads_.resize(thread_num);
      flags_.resize(thread_num);

      for (int i = old_thread_num; i < thread_num; ++i) {
        flags_[i] = std::make_shared<std::atomic<bool>>(false);
        this->setThread(i);
      }
    } else {  // the number of threads is decreased
      for (int i = old_thread_num - 1; i >= thread_num; --i) {
        *flags_[i] = true;  // this thread will finish
        threads_[i]->detach();
      }
      {
        // stop the detached threads that were waiting
        std::unique_lock lock(mutex_);
        cv_.notify_all();
      }
      threads_.resize(
        thread_num);  // safe to delete because the threads are detached
      flags_.resize(
        thread_num);  // safe to delete because the threads have copies of
                      // shared_ptr of the flags, not originals
    }
  }
}

// empty the queue
void thread_pool::clearQueue() {
  std::function<void(int id)> *_f;
  while (q_.try_dequeue(_f)) {
    delete _f;  // empty the queue
  }
}

// pops a functional wrapper to the original function
std::function<void(int)> thread_pool::pop() {
  std::function<void(int id)> *_f = nullptr;
  q_.try_dequeue(_f);
  std::unique_ptr<std::function<void(int id)>> func(
    _f);  // at return, delete the function even if an exception occurred

  std::function<void(int)> f;
  if (_f) {
    f = *_f;
  }
  return f;
}

// wait for all computing threads to finish and stop all threads
// may be called asynchronously to not pause the calling thread while waiting
// if wait == true, all the functions in the queue are run, otherwise the
// queue is cleared without running the functions
void thread_pool::stop(bool wait) {
  if (!wait) {
    if (stop_) {
      return;
    }
    stop_ = true;
    for (int i = 0, n = this->getSize(); i < n; ++i) {
      *flags_[i] = true;  // command the threads to stop
    }
    this->clearQueue();  // empty the queue
  } else {
    if (done_ || stop_) {
      return;
    }
    done_ = true;  // give the waiting threads a command to finish
  }
  {
    std::unique_lock lock(mutex_);
    cv_.notify_all();  // stop all waiting threads
  }
  for (const auto &thread : threads_) {
    if (thread->joinable()) {
      thread->join();
    }
  }
  // if there were no threads in the pool but some functors in the queue, the
  // functors are not deleted by the threads therefore delete them here
  this->clearQueue();
  threads_.clear();
  flags_.clear();
}

void thread_pool::setThread(int i) {
  std::shared_ptr<std::atomic<bool>> flag(
    flags_[i]);  // a copy of the shared ptr to the flag
  auto f = [this, i, flag /* a copy of the shared ptr to the flag */]() {
    amdinfer::util::setThreadName("ctplPool");
    const std::atomic<bool> &_flag = *flag;
    std::function<void(int id)> *_f;
    bool isPop = q_.try_dequeue(_f);
    while (true) {
      while (isPop) {  // if there is anything in the queue
        // at return, delete the function even if an exception occurred
        std::unique_ptr<std::function<void(int id)>> func(_f);
        (*_f)(i);

        // the thread is wanted to stop, return even if queue is not empty yet
        if (_flag) {
          return;
        } else {
          isPop = q_.try_dequeue(_f);
        }
      }

      // the queue is empty here, wait for the next command
      std::unique_lock lock(mutex_);
      ++waiting_;
      cv_.wait(lock, [this, &_f, &isPop, &_flag]() {
        isPop = q_.try_dequeue(_f);
        return isPop || done_ || _flag;
      });
      --waiting_;

      // if the queue is empty and done_ == true or *flag then return
      if (!isPop) {
        return;
      }
    }
  };
  // compiler may not support std::make_unique()
  threads_[i].reset(new std::thread(f));
}

}  // namespace amdinfer::util
