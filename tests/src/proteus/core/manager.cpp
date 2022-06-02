// Copyright 2021 Xilinx Inc.
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
 * @brief Implements how the shared mutable state of Proteus is managed as
 * Proteus runs
 */

#include "proteus/core/manager.hpp"

#include <stdexcept>  // for invalid_argument
#include <thread>     // for thread
#include <utility>    // for pair, make_pair, move

#include "proteus/build_options.hpp"     // for PROTEUS_ENABLE_LOGGING
#include "proteus/core/worker_info.hpp"  // for WorkerInfo
#include "proteus/workers/worker.hpp"    // for Worker

namespace proteus {

Manager::Manager() {}

std::string Manager::loadWorker(std::string const& key,
                                RequestParameters parameters) {
  auto endpoint = this->endpoints_.add(key, parameters);
  return endpoint;
}

void Manager::unloadWorker(const std::string& key) {
  if (this->endpoints_.exists(key)) {
    this->endpoints_.unload(key);
  }
}

WorkerInfo* Manager::getWorker(const std::string& key) {
  auto* worker_info = this->endpoints_.get(key);
  if (worker_info == nullptr) {
    throw std::invalid_argument("worker " + key + " not found");
  }
  return worker_info;
}

bool Manager::workerReady(const std::string& key) {
  auto metadata = this->getWorkerMetadata(key);
  return metadata.isReady();
}

ModelMetadata Manager::getWorkerMetadata(const std::string& key) {
  auto* worker = this->getWorker(key);
  auto* foo = worker->workers_.begin()->second;
  return foo->getMetadata();
}

void Manager::workerAllocate(std::string const& key, int num) {
  auto* worker_info = this->endpoints_.get(key);
  if (!worker_info->inputSizeValid(num)) {
    PROTEUS_LOG_DEBUG(logger_, "Allocating more buffers for worker " + key);
    worker_info->allocate(num);
  }
}

void Manager::shutdown() {
  auto request = std::make_shared<UpdateCommand>(UpdateCommandType::Shutdown);
  this->update_queue_->enqueue(request);
  if (this->update_thread_.joinable()) {
    this->update_thread_.join();
  }
}

void Manager::update_manager(UpdateCommandQueue* input_queue) {
  (void)input_queue;
}

std::string Manager::Endpoints::load(const std::string& worker,
                                     RequestParameters* parameters) {
  if (worker_endpoints_.find(worker) == worker_endpoints_.end()) {
    // this is a brand-new worker we haven't seen before
    std::map<RequestParameters, std::string> map;
    map.insert(std::make_pair(*parameters, worker));
    worker_endpoints_.insert(std::make_pair(worker, map));
    worker_parameters_.insert(std::make_pair(worker, *parameters));
    return worker;
  }

  auto& map = worker_endpoints_.at(worker);

  // we've seen this worker before but not with these parameters
  if (map.find(*parameters) == map.end()) {
    int index = 0;
    if (worker_indices_.find(worker) != worker_indices_.end()) {
      // technically, this can overflow and cause problems but that's unlikely
      index = worker_indices_.at(worker) + 1;
    }
    std::string url = worker + "-" + std::to_string(index);
    map.insert(std::make_pair(*parameters, url));

    worker_indices_.insert_or_assign(worker, index);
    worker_parameters_.insert(std::make_pair(url, *parameters));
    return url;
  }
  return map.at(*parameters);
}

void Manager::Endpoints::unload(const std::string& endpoint) {
  auto hyphen_pos = endpoint.find('-');
  auto worker =
    hyphen_pos != std::string::npos ? endpoint.substr(0, hyphen_pos) : endpoint;

  auto* worker_info = this->get(endpoint);
  if (worker_info != nullptr) {
    worker_info->unload();
  }

  // if it's a brand-new worker that failed or the last worker being unloaded,
  // clean up our parameters and endpoint metadata
  if (worker_info == nullptr || worker_info->getGroupSize() == 0) {
    this->workers_.erase(endpoint);

    if (worker_endpoints_.find(worker) != worker_endpoints_.end()) {
      auto& map = worker_endpoints_.at(worker);
      auto& parameters = worker_parameters_.at(endpoint);

      map.erase(parameters);
      if (map.empty()) {
        worker_endpoints_.erase(worker);
        worker_indices_.erase(worker);
      }
      worker_parameters_.erase(endpoint);
    }
  }
}

bool Manager::Endpoints::exists(const std::string& endpoint) {
  return workers_.find(endpoint) != workers_.end();
}

WorkerInfo* Manager::Endpoints::get(const std::string& endpoint) {
  auto iterator = workers_.find(endpoint);
  if (iterator != workers_.end()) {
    return iterator->second.get();
  }
  return nullptr;
}

std::string Manager::Endpoints::add(const std::string& worker,
                                    RequestParameters parameters) {
  bool share = true;
  if (parameters.has("share")) {
    share = parameters.get<bool>("share");
    parameters.erase("share");
  }

  auto endpoint = this->load(worker, &parameters);
  auto* worker_info = this->get(endpoint);

  // if the worker doesn't exist yet, we need to create it
  try {
    if (worker_info == nullptr) {
      auto new_worker = std::make_unique<WorkerInfo>(endpoint, &parameters);
      this->workers_.insert(std::make_pair(endpoint, std::move(new_worker)));
      // if the worker exists but the share parameter is false, we need to add
      // one
    } else if (!share) {
      worker_info->addAndStartWorker(endpoint, &parameters);
    }
  } catch (...) {
    // undo the load if the worker creation fails
    this->unload(endpoint);
    throw;
  }
  return endpoint;
}

void Manager::Endpoints::shutdown() {
  for (auto const& worker_info : this->workers_) {
    worker_info.second->shutdown();
  }
  this->workers_.clear();
  this->worker_endpoints_.clear();
  this->worker_indices_.clear();
  this->worker_parameters_.clear();
}

}  // namespace proteus
