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

/**
 * @file
 * @brief Implements the WorkerInfo class - information the Manager
 * saves on each active worker
 */

#include "amdinfer/core/worker_info.hpp"

#include <dlfcn.h>  // for dlerror, dlopen, dlsym, RTL...

#include <cctype>       // for toupper
#include <climits>      // for UINT_MAX
#include <cstdint>      // for int32_t
#include <exception>    // for exception
#include <string>       // for string, operator+, basic_st...
#include <type_traits>  // for remove_reference<>::type
#include <utility>      // for pair, move, make_pair

#include "amdinfer/batching/batcher.hpp"  // for Batcher, BatcherStatus, Bat...
#include "amdinfer/core/exceptions.hpp"   // for invalid_argument, external_...
#include "amdinfer/core/memory_pool/pool.hpp"   // for MemoryPool
#include "amdinfer/core/parameters.hpp"         // for ParameterMap
#include "amdinfer/core/request_container.hpp"  // for ModelMetadata
#include "amdinfer/workers/worker.hpp"  // for Worker, WorkerStatus, Worke...

namespace amdinfer {

void* getHandle(const std::string& name) {
  // multiple workers with different configurations may exist. Remove the config
  // tag that starts with "-" in the name prior to loading the .so
  auto lib_name = name;
  lib_name[0] = static_cast<char>(std::toupper(lib_name[0]));
  if (auto hyphen_pos = name.find('-'); hyphen_pos != std::string::npos) {
    lib_name.erase(hyphen_pos);
  }
  std::string library =
    std::string("libworker") + lib_name + std::string(".so");

  // reset errors
  dlerror();

  /*
  Open the needed object. The dlopen flags used here:
    - RTLD_LOCAL: the symbols are not made available to other loaded libs
    - RTLD_LAZY: resolve symbols as needed. We only need one anyway
  Adding RTLD_DEEPBIND here creates problems:
    - Cannot use std::cout in the library
      (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=42679)
    - std::regex gives a segfault
  There are many SO posts reporting problems related to issues with DEEPBIND
  The motivation to add DEEPBIND is to isolate the loaded workers. For example,
  if the library is using a different version of a library that we are already
  using, it can link to the wrong version. Another option for isolating the
  workers is dlmopen but that also should not be used here due to its own set of
  issues (https://sourceware.org/bugzilla/show_bug.cgi?id=24776).
  */
  void* handle = dlopen(library.c_str(), RTLD_LOCAL | RTLD_LAZY);
  return handle;
}

/**
 * @brief Find the named function in a *.so file
 *
 * @param func name of the symbol  to find
 * @param so_path path to the *.so file to search
 * @return void* pointer to the function
 */
void* findFunc(void* handle, const std::string& func) {
  // reset errors
  dlerror();

  if (handle == nullptr) {
    const char* error_str = dlerror();
    throw file_not_found_error(error_str);
  }

  /* find the address of function  */
  void* fptr = dlsym(handle, func.c_str());
  if (fptr == nullptr) {
    const char* error_str = dlerror();
    throw invalid_argument(error_str);
  }
  return fptr;
}

workers::Worker* getWorker(void* handle) {
  void* func_ptr = findFunc(handle, "getWorker");

  // cast the void pointer from dlsym to a function pointer. This assumes that
  // void* is same size as function pointer, which should be true on POSIX
  auto* worker = reinterpret_cast<workers::Worker* (*)()>(func_ptr)();
  return worker;
}

WorkerInfo::WorkerInfo(const std::string& name, ParameterMap* parameters,
                       MemoryPool* pool, BatchPtrQueue* next,
                       const std::vector<MemoryAllocators>& next_allocators)
  : next_(next), next_allocators_(next_allocators) {
  handle_ = getHandle(name);
  this->addAndStartWorker(name, parameters, pool);
}

WorkerInfo::~WorkerInfo() {
  batchers_.clear();
  for (const auto& [thread_id, worker] : workers_) {
    delete worker;  // NOLINT(cppcoreguidelines-owning-memory)
  }
  dlclose(handle_);
  handle_ = nullptr;
}

void WorkerInfo::addAndStartWorker(const std::string& name,
                                   ParameterMap* parameters, MemoryPool* pool) {
  auto* worker = getWorker(handle_);
  worker->init(parameters);

  std::vector<MemoryAllocators> allocators = worker->getAllocators();
  ;

  try {
    worker->acquire(parameters);
  } catch (const std::exception& e) {
    throw external_error(e.what());
  } catch (...) {
    throw runtime_error("Unknown error occurred");
  }

  this->batch_size_ = worker->getBatchSize();
  worker->setNext(next_);
  worker->setNextAllocators(next_allocators_);

  if (this->batchers_.empty()) {
    int32_t batcher_count = 1;
    if (parameters->has("batchers")) {
      batcher_count = parameters->get<int32_t>("batchers");
    }
    this->batchers_ = worker->makeBatcher(batcher_count, parameters, pool);

    for (const auto& batcher : this->batchers_) {
      batcher->setName(name);
      batcher->setBatchSize(this->batch_size_);
    }
  }

  for (const auto& batcher : this->batchers_) {
    if (batcher->getStatus() != BatcherStatus::Run) {
      batcher->start(allocators);
    }
  }
  std::thread thread{&workers::Worker::run, worker,
                     this->batchers_[0]->getOutputQueue(), pool};

  auto thread_id = thread.get_id();

  this->worker_threads_.insert(std::make_pair(thread_id, std::move(thread)));
  this->workers_.insert(std::make_pair(thread_id, worker));
}

Batcher* WorkerInfo::getBatcher() { return this->batchers_[0].get(); }

BatchPtrQueue* WorkerInfo::getInputQueue() const {
  return batchers_[0]->getOutputQueue();
}

void WorkerInfo::join(std::thread::id id) {
  auto& thread = worker_threads_.at(id);
  if (thread.joinable()) {
    thread.join();
  }
}

void WorkerInfo::joinAll() {
  for (auto& [thread_id, thread] : worker_threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void WorkerInfo::unload() {
  this->batchers_[0]->getOutputQueue()->enqueue(nullptr);

  bool last_worker = this->workers_.size() == 1;
  if (last_worker) {
    this->joinAll();
    // we enqueue nullptrs to kill the batchers but don't know which batcher
    // may receive the nullptr if there are multiple. So, we loop through all of
    // of them until we find one that's inactive, indicating it received the
    // nullptr, and end that one.
    for (const auto& batcher : this->batchers_) {
      batcher->enqueue(nullptr);
      auto i = -1;
      BatcherStatus status;
      do {
        i = (i + 1) % this->batchers_.size();
        status = batchers_[i]->getStatus();
      } while (status != BatcherStatus::Inactive);
      this->batchers_[i]->end();
    }
  }

  std::thread::id id;
  bool found = false;
  while (!found) {
    for (const auto& [thread_id, worker] : this->workers_) {
      if (worker->getStatus() == workers::WorkerStatus::Inactive) {
        id = thread_id;
        found = true;
        this->join(thread_id);
        break;
      }
    }
  }
  auto* worker = this->workers_[id];
  worker->release();
  worker->destroy();

  if (last_worker) {
    delete worker;  // NOLINT(cppcoreguidelines-owning-memory)
  }
  this->workers_.erase(id);
}

size_t WorkerInfo::getGroupSize() const { return this->workers_.size(); }

void WorkerInfo::shutdown() {
  auto workers = this->getGroupSize();
  for (auto i = 0U; i < workers; i++) {
    this->unload();
  }
}

ModelMetadata WorkerInfo::getMetadata() const {
  auto* worker_class = workers_.begin()->second;
  return worker_class->getMetadata();
}

std::vector<MemoryAllocators> WorkerInfo::getAllocators() const {
  const auto* worker_class = workers_.begin()->second;
  return worker_class->getAllocators();
}

}  // namespace amdinfer
