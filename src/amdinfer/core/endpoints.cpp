// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief
 */

#include "amdinfer/core/endpoints.hpp"

#include <cassert>      // for assert
#include <type_traits>  // for __decay_and_strip<>::__type

#include "amdinfer/batching/batcher.hpp"  // for Batcher
#include "amdinfer/build_options.hpp"     // for kMaxModelNameSize
#include "amdinfer/core/exceptions.hpp"   // for invalid_argument
#include "amdinfer/core/interface.hpp"    // IWYU pragma: keep
#include "amdinfer/core/parameters.hpp"   // for ParameterMap
#include "amdinfer/core/worker_info.hpp"  // for WorkerInfo
#include "amdinfer/util/thread.hpp"       // for setThreadName

namespace amdinfer {

Endpoints::Endpoints() {
  update_thread_ = std::thread(&Endpoints::updateManager, this, &update_queue_);
}

Endpoints::~Endpoints() { this->shutdown(); }

std::string Endpoints::load(const std::string& worker,
                            ParameterMap parameters) {
  std::shared_ptr<amdinfer::UpdateCommand> request;
  std::string retval;
  retval.reserve(kMaxModelNameSize);
  retval = "";
  request = std::make_shared<UpdateCommand>(UpdateCommandType::Load, worker,
                                            &parameters, &retval);
  update_queue_.enqueue(request);

  while (static_cast<std::string*>(request->retval)->empty() &&
         request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }
  auto endpoint = *(static_cast<std::string*>(request->retval));
  return endpoint;
}

void Endpoints::unload(const std::string& endpoint) {
  if (this->exists(endpoint)) {
    auto request =
      std::make_shared<UpdateCommand>(UpdateCommandType::Unload, endpoint);
    update_queue_.enqueue(request);
  }
}

// TODO(varunsh): race condition if workers are shutting down
void Endpoints::infer(const std::string& endpoint,
                      std::unique_ptr<Interface> request) const {
  WorkerInfo* worker = this->unsafeGet(endpoint);
  if (worker == nullptr) {
    throw invalid_argument("Worker " + endpoint + " not found");
  }
  auto* batcher = worker->getBatcher();
  batcher->enqueue(std::move(request));
}

bool Endpoints::exists(const std::string& endpoint) {
  int retval = -1;
  auto request = std::make_shared<UpdateCommand>(UpdateCommandType::Exists,
                                                 endpoint, nullptr, &retval);
  update_queue_.enqueue(request);

  while (retval == -1 && request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }
  return retval != 0;
}

bool Endpoints::ready(const std::string& endpoint) {
  std::shared_ptr<amdinfer::UpdateCommand> request;
  int ready = -1;
  request = std::make_shared<UpdateCommand>(UpdateCommandType::Ready, endpoint,
                                            nullptr, &ready);
  update_queue_.enqueue(request);
  while (ready == -1 && request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }
  return ready != 0;
}

std::vector<std::string> Endpoints::list() {
  std::shared_ptr<amdinfer::UpdateCommand> request;
  std::vector<std::string> endpoints;
  bool retval = false;
  request = std::make_shared<UpdateCommand>(UpdateCommandType::List, "",
                                            &endpoints, &retval);
  update_queue_.enqueue(request);
  while (!retval && request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }
  return endpoints;
}

ModelMetadata Endpoints::metadata(const std::string& endpoint) {
  std::shared_ptr<amdinfer::UpdateCommand> request;
  ModelMetadata metadata{"", ""};
  bool retval = false;
  request = std::make_shared<UpdateCommand>(UpdateCommandType::Metadata,
                                            endpoint, &metadata, &retval);
  update_queue_.enqueue(request);
  while (!retval && request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }
  return metadata;
}

// TODO(varunsh): if multiple commands sent post-shutdown, they will linger
// in the queue and may cause problems
void Endpoints::shutdown() {
  if (this->update_thread_.joinable()) {
    auto request = std::make_shared<UpdateCommand>(UpdateCommandType::Shutdown);
    update_queue_.enqueue(request);
    this->update_thread_.join();
  }
}

void Endpoints::updateManager(UpdateCommandQueue* input_queue) {
  AMDINFER_LOG_DEBUG(logger_, "Starting the Manager update thread");
  util::setThreadName("manager");
  std::shared_ptr<UpdateCommand> request;
  bool run = true;
  while (run) {
    input_queue->wait_dequeue(request);
    AMDINFER_LOG_DEBUG(logger_,
                       "Got request in Manager update thread with ID " +
                         std::to_string(static_cast<int>(request->cmd)));
    switch (request->cmd) {
      case UpdateCommandType::Load:
        try {
          auto* parameters = static_cast<ParameterMap*>(request->object);
          auto endpoint = this->unsafeLoad(request->key, parameters);
          static_cast<std::string*>(request->retval)
            ->assign(std::string{endpoint});
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
      case UpdateCommandType::Unload:
        this->unsafeUnload(request->key);
        break;
      case UpdateCommandType::Exists:
        try {
          *static_cast<int*>(request->retval) =
            static_cast<int>(this->unsafeExists(request->key));
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
      case UpdateCommandType::Ready:
        try {
          auto metadata = this->unsafeMetadata(request->key);
          *static_cast<int*>(request->retval) =
            static_cast<int>(metadata.isReady());
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
      case UpdateCommandType::List:
        try {
          auto* list = static_cast<std::vector<std::string>*>(request->object);
          this->unsafeList(list);
          *static_cast<bool*>(request->retval) = true;
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
      case UpdateCommandType::Metadata:
        try {
          auto* metadata = static_cast<ModelMetadata*>(request->object);
          *metadata = this->unsafeMetadata(request->key);
          *static_cast<bool*>(request->retval) = true;
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
      case UpdateCommandType::Shutdown:
        this->unsafeShutdown();
        run = false;
        break;
    }
  }
  AMDINFER_LOG_DEBUG(logger_, "Ending update_thread");
}

std::string Endpoints::unsafeLoad(const std::string& worker,
                                  ParameterMap* parameters) {
  bool share = true;
  if (parameters->has("share")) {
    share = parameters->get<bool>("share");
    parameters->erase("share");
  }

  auto endpoint = this->insertWorker(worker, *parameters);
  auto* worker_info = this->unsafeGet(endpoint);

  std::string worker_name = endpoint;
  if (parameters->has("worker")) {
    worker_name = parameters->get<std::string>("worker");
  }

  // if the worker doesn't exist yet, we need to create it
  try {
    if (worker_info == nullptr) {
      auto new_worker =
        std::make_unique<WorkerInfo>(worker_name, parameters, &pool_);
      this->workers_.try_emplace(endpoint, std::move(new_worker));
      // if the worker exists but the share parameter is false, we need to add
      // one
    } else if (!share) {
      worker_info->addAndStartWorker(worker_name, parameters, &pool_);
    }
  } catch (...) {
    // undo the load if the worker creation fails
    this->unsafeUnload(endpoint);
    throw;
  }
  return endpoint;
}

void Endpoints::unsafeUnload(const std::string& endpoint) {
  auto hyphen_pos = endpoint.find('-');
  auto worker =
    hyphen_pos != std::string::npos ? endpoint.substr(0, hyphen_pos) : endpoint;

  auto* worker_info = this->unsafeGet(endpoint);
  if (worker_info != nullptr) {
    worker_info->unload();
  }

  // if it's a brand-new worker that failed or the last worker being unloaded,
  // clean up our parameters and endpoint metadata
  if (worker_info == nullptr || worker_info->getGroupSize() == 0) {
    this->workers_.erase(endpoint);

    if (worker_endpoints_.find(worker) != worker_endpoints_.end()) {
      auto& map = worker_endpoints_.at(worker);
      if (worker_parameters_.find(endpoint) != worker_parameters_.end()) {
        const auto& parameters = worker_parameters_.at(endpoint);
        map.erase(parameters);
      }
      if (map.empty()) {
        worker_endpoints_.erase(worker);
        worker_indices_.erase(worker);
      }
      worker_parameters_.erase(endpoint);
    }
  }
}

bool Endpoints::unsafeExists(const std::string& endpoint) const {
  return workers_.find(endpoint) != workers_.end();
}

WorkerInfo* Endpoints::unsafeGet(const std::string& endpoint) const {
  if (auto iterator = workers_.find(endpoint); iterator != workers_.end()) {
    return iterator->second.get();
  }
  return nullptr;
}

void Endpoints::unsafeList(std::vector<std::string>* list) const {
  assert(list != nullptr);
  assert(list->empty());
  list->reserve(workers_.size());
  for (const auto& [worker, _] : workers_) {
    list->push_back(worker);
  }
}

// FIXME(varunsh): potential race condition if the worker is being deleted
ModelMetadata Endpoints::unsafeMetadata(const std::string& endpoint) const {
  const auto* worker = this->unsafeGet(endpoint);
  if (worker == nullptr) {
    throw invalid_argument("Worker " + endpoint + " not found");
  }
  return worker->getMetadata();
}

void Endpoints::unsafeShutdown() {
  for (auto const& worker_info : this->workers_) {
    worker_info.second->shutdown();
  }
  this->workers_.clear();
  this->worker_endpoints_.clear();
  this->worker_indices_.clear();
  this->worker_parameters_.clear();
}

std::string Endpoints::insertWorker(const std::string& worker,
                                    const ParameterMap& parameters) {
  if (worker_endpoints_.find(worker) == worker_endpoints_.end()) {
    // this is a brand-new worker we haven't seen before
    std::map<ParameterMap, std::string> map;
    map.insert(std::make_pair(parameters, worker));
    worker_endpoints_.insert(std::make_pair(worker, map));
    worker_parameters_.insert(std::make_pair(worker, parameters));
    return worker;
  }

  auto& map = worker_endpoints_.at(worker);

  // we've seen this worker before but not with these parameters
  if (map.find(parameters) == map.end()) {
    int index = 0;
    if (worker_indices_.find(worker) != worker_indices_.end()) {
      // technically, this can overflow and cause problems but that's unlikely
      index = worker_indices_.at(worker) + 1;
    }
    std::string url = worker + "-" + std::to_string(index);
    map.insert(std::make_pair(parameters, url));

    worker_indices_.insert_or_assign(worker, index);
    worker_parameters_.insert(std::make_pair(url, parameters));
    return url;
  }
  return map.at(parameters);
}

}  // namespace amdinfer
