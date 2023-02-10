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
#include "amdinfer/core/interface.hpp"    // IWYU pragma: keep
#include "amdinfer/core/parameters.hpp"   // for ParameterMap
#include "amdinfer/core/predict_api.hpp"  // for ModelMetadata
#include "amdinfer/workers/worker.hpp"    // for Worker, WorkerStatus, Worke...

namespace amdinfer {

/**
 * @brief Find the named function in a *.so file
 *
 * @param func name of the symbol  to find
 * @param so_path path to the *.so file to search
 * @return void* pointer to the function
 */
void* findFunc(const std::string& func, const std::string& so_path) {
  if (func.empty() || so_path.empty()) {
    throw invalid_argument("Function or .so path empty");
  }

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
  void* handle = dlopen(so_path.c_str(), RTLD_LOCAL | RTLD_LAZY);
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

workers::Worker* getWorker(const std::string& name) {
  // multiple workers with different configurations may exist. Remove the config
  // tag that starts with "-" in the name prior to loading the .so
  auto lib_name = name;
  lib_name[0] = static_cast<char>(std::toupper(lib_name[0]));
  if (auto hyphen_pos = name.find('-'); hyphen_pos != std::string::npos) {
    lib_name.erase(hyphen_pos);
  }
  std::string library =
    std::string("libworker") + lib_name + std::string(".so");

  void* func_ptr = findFunc("getWorker", library);

  // cast the void pointer from dlsym to a function pointer. This assumes that
  // void* is same size as function pointer, which should be true on POSIX
  auto* worker = reinterpret_cast<workers::Worker* (*)()>(func_ptr)();
  return worker;
}

WorkerInfo::WorkerInfo(const std::string& name, ParameterMap* parameters) {
  this->input_buffer_ptr_ = std::make_unique<BufferPtrsQueue>();
  this->output_buffer_ptr_ = std::make_unique<BufferPtrsQueue>();

  this->addAndStartWorker(name, parameters);
}

WorkerInfo::~WorkerInfo() {
  batchers_.clear();
  for (const auto& [thread_id, worker] : workers_) {
    delete worker;  // NOLINT(cppcoreguidelines-owning-memory)
  }
}

void WorkerInfo::addAndStartWorker(const std::string& name,
                                   ParameterMap* parameters) {
  auto* worker = getWorker(name);
  worker->init(parameters);
  this->batch_size_ = worker->getBatchSize();

  if (this->batchers_.empty()) {
    int32_t batcher_count = 1;
    if (parameters->has("batchers")) {
      batcher_count = parameters->get<int32_t>("batchers");
    }
    this->batchers_ = worker->makeBatcher(batcher_count, parameters);

    for (const auto& batcher : this->batchers_) {
      batcher->setName(name);
      batcher->setBatchSize(this->batch_size_);
    }
  }

  auto max_buffers = worker->getMaxBufferNum();
  worker->setInputBuffers(this->input_buffer_ptr_.get());
  worker->setOutputBuffers(this->output_buffer_ptr_.get());
  try {
    auto buffer_num = worker->allocate(kNumBufferAuto);
    this->buffer_num_ += buffer_num;
  } catch (const std::exception& e) {
    if (workers_.empty()) {
      this->batchers_.clear();
    }
    throw external_error(e.what());
  } catch (...) {
    if (workers_.empty()) {
      this->batchers_.clear();
    }
    throw runtime_error("Unknown error occurred");
  }
  this->max_buffer_num_ =
    max_buffers == UINT_MAX ? UINT_MAX : this->max_buffer_num_ + max_buffers;

  try {
    worker->acquire(parameters);
  } catch (const std::exception& e) {
    if (workers_.empty()) {
      this->batchers_.clear();
    }
    throw external_error(e.what());
  } catch (...) {
    if (workers_.empty()) {
      this->batchers_.clear();
    }
    throw runtime_error("Unknown error occurred");
  }

  for (const auto& batcher : this->batchers_) {
    if (batcher->getStatus() != BatcherStatus::Run) {
      batcher->start(this);
    }
  }
  auto thread = worker->spawn(this->batchers_[0]->getOutputQueue());

  auto thread_id = thread.get_id();

  this->worker_threads_.insert(std::make_pair(thread_id, std::move(thread)));
  this->workers_.insert(std::make_pair(thread_id, worker));
}

Batcher* WorkerInfo::getBatcher() { return this->batchers_[0].get(); }

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
  worker->deallocate();
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

BufferPtrs WorkerInfo::getInputBuffer() const {
  BufferPtrs buffer;
  input_buffer_ptr_->wait_dequeue(buffer);
  return buffer;
}

BufferPtrs WorkerInfo::getOutputBuffer() const {
  BufferPtrs buffer;
  output_buffer_ptr_->wait_dequeue(buffer);
  return buffer;
}

void WorkerInfo::putInputBuffer(BufferPtrs&& buffer) const {
  this->input_buffer_ptr_->enqueue(std::move(buffer));
}

void WorkerInfo::putOutputBuffer(BufferPtrs&& buffer) const {
  this->output_buffer_ptr_->enqueue(std::move(buffer));
}

bool WorkerInfo::inputSizeValid(size_t size) const {
  if (size <= this->getBufferNum()) {
    return true;
  }
  if (size <= this->getMaxBufferNum()) {
    return false;
  }
  throw invalid_argument("Too many input tensors for this model");
}

void WorkerInfo::allocate(size_t request_size) {
  // TODO(varunsh): can result in deadlock in manager if allocated num !=
  // request
  auto allocated =
    this->workers_.begin()->second->allocate(request_size - this->buffer_num_);
  this->buffer_num_ += allocated;
}

ModelMetadata WorkerInfo::getMetadata() const {
  auto* worker_class = workers_.begin()->second;
  return worker_class->getMetadata();
}

}  // namespace amdinfer
