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
#include <thread>     // for yield, thread
#include <utility>    // for pair, make_pair, move

#include "proteus/build_options.hpp"     // for PROTEUS_ENABLE_LOGGING, kMax...
#include "proteus/core/worker_info.hpp"  // for WorkerInfo
#include "proteus/helpers/thread.hpp"    // for setThreadName
#include "proteus/workers/worker.hpp"    // for Worker

namespace proteus {

Manager::Manager() {
  update_queue_ = std::make_unique<UpdateCommandQueue>();
  init();
}

void Manager::init() {
  // default constructed threads are not joinable
  if (!update_thread_.joinable()) {
    update_thread_ =
      std::thread(&Manager::update_manager, this, update_queue_.get());
  }
}

std::string Manager::loadWorker(std::string const& key,
                                RequestParameters parameters) {
  std::shared_ptr<proteus::UpdateCommand> request;
  std::string retval;
  retval.reserve(kMaxModelNameSize);
  retval = "";
  request = std::make_shared<UpdateCommand>(UpdateCommandType::Add, key,
                                            &parameters, &retval);
  update_queue_->enqueue(request);

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

void Manager::unloadWorker(const std::string& key) {
  if (this->endpoints_.exists(key)) {
    auto request =
      std::make_shared<UpdateCommand>(UpdateCommandType::Delete, key);
    update_queue_->enqueue(request);
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
  std::shared_ptr<proteus::UpdateCommand> request;
  int ready = -1;
  request = std::make_shared<UpdateCommand>(UpdateCommandType::Ready, key,
                                            nullptr, &ready);
  update_queue_->enqueue(request);
  while (ready == -1 && request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }
  return ready;
}

// FIXME(varunsh): potential race condition if the worker is being deleted
ModelMetadata Manager::getWorkerMetadata(const std::string& key) {
  auto* worker = this->getWorker(key);
  auto* foo = worker->workers_.begin()->second;
  return foo->getMetadata();
}

void Manager::workerAllocate(std::string const& key, int num) {
  const auto* worker = this->getWorker(key);
  auto request =
    std::make_shared<UpdateCommand>(UpdateCommandType::Allocate, key, &num);
  update_queue_->enqueue(request);
  while (!worker->inputSizeValid(num) && request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }
}

std::vector<std::string> Manager::getWorkerEndpoints() {
  return this->endpoints_.list();
}

// TODO(varunsh): if multiple commands sent post-shutdown, they will linger
// in the queue and may cause problems
void Manager::shutdown() {
  auto request = std::make_shared<UpdateCommand>(UpdateCommandType::Shutdown);
  this->update_queue_->enqueue(request);
  if (this->update_thread_.joinable()) {
    this->update_thread_.join();
  }
}

void Manager::update_manager(UpdateCommandQueue* input_queue) {
  PROTEUS_LOG_DEBUG(logger_, "Starting the Manager update thread");
  setThreadName("manager");
  std::shared_ptr<UpdateCommand> request;
  bool run = true;
  while (run) {
    input_queue->wait_dequeue(request);
    PROTEUS_LOG_DEBUG(logger_, "Got request in Manager update thread");
    switch (request->cmd) {
      case UpdateCommandType::Shutdown:
        this->endpoints_.shutdown();
        run = false;
        break;
      case UpdateCommandType::Delete:
        this->endpoints_.unload(request->key);
        break;
      case UpdateCommandType::Allocate:
        try {
          auto* worker_info = this->endpoints_.get(request->key);
          auto num = *static_cast<int*>(request->object);
          if (!worker_info->inputSizeValid(num)) {
            PROTEUS_LOG_DEBUG(
              logger_,

              "Allocating more buffers for worker " + request->key);
            worker_info->allocate(num);
          }
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
      case UpdateCommandType::Add:
        try {
          auto* parameters = static_cast<RequestParameters*>(request->object);
          auto endpoint = this->endpoints_.add(request->key, *parameters);
          static_cast<std::string*>(request->retval)
            ->assign(std::string{endpoint});
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
      case UpdateCommandType::Ready:
        try {
          auto* workerInfo = this->getWorker(request->key);
          auto* worker = workerInfo->workers_.begin()->second;
          auto metadata = worker->getMetadata();
          *static_cast<int*>(request->retval) = metadata.isReady();
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
    }
  }
  PROTEUS_LOG_DEBUG(logger_, "Ending update_thread");
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

bool Manager::Endpoints::exists(const std::string& endpoint) {
  return workers_.find(endpoint) != workers_.end();
}

std::vector<std::string> Manager::Endpoints::list() {
  std::vector<std::string> workers;
  workers.reserve(this->workers_.size());
  for (const auto& [worker, _] : workers_) {
    workers.push_back(worker);
  }
  return workers;
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

  std::string worker_name = endpoint;
  if (parameters.has("worker")) {
    worker_name = parameters.get<std::string>("worker");
  }

  // if the worker doesn't exist yet, we need to create it
  try {
    if (worker_info == nullptr) {
      auto new_worker = std::make_unique<WorkerInfo>(worker_name, &parameters);
      this->workers_.insert(std::make_pair(endpoint, std::move(new_worker)));
      // if the worker exists but the share parameter is false, we need to add
      // one
    } else if (!share) {
      worker_info->addAndStartWorker(worker_name, &parameters);
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
