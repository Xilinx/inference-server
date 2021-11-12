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

#include "proteus/batching/batcher.hpp"  // for Batcher
#include "proteus/build_options.hpp"     // for PROTEUS_ENABLE_LOGGING, kMax...
#include "proteus/core/worker_info.hpp"  // for WorkerInfo
#include "proteus/helpers/thread.hpp"    // for setThreadName
#include "proteus/workers/worker.hpp"    // for Worker

namespace proteus {

Manager::Manager() {
#ifdef PROTEUS_ENABLE_LOGGING
  this->logger_ = getLogger();
#endif
  update_queue_ = std::make_unique<UpdateCommandQueue>();
  update_thread_ =
    std::thread(&Manager::update_manager, this, update_queue_.get());
}

std::string Manager::loadWorker(std::string const& key,
                                RequestParameters* parameters) {
  bool share = true;
  if (parameters->has("share")) {
    share = parameters->get<bool>("share");
  } else {
    parameters->put("share", share);
  }
  // TODO(varunsh): if we modify the passed parameters, we can't reuse it
  // parameters->erase("share");

  std::shared_ptr<proteus::UpdateCommand> request;
  std::string retval;
  retval.reserve(kMaxModelNameSize);
  retval = "";
  request = std::make_shared<UpdateCommand>(UpdateCommandType::Load, key,
                                            parameters, &retval);
  update_queue_->enqueue(request);

  while (static_cast<std::string*>(request->retval)->empty() &&
         request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }
  auto endpoint = *(static_cast<std::string*>(request->retval));

  bool creating = true;
  if (!workerExists(endpoint)) {
    request = std::make_shared<UpdateCommand>(UpdateCommandType::Create,
                                              endpoint, parameters, &creating);
  } else if (!share) {
    request = std::make_shared<UpdateCommand>(UpdateCommandType::Duplicate,
                                              endpoint, parameters, &creating);
  } else {
    // share is true and worker already exists. This is a superfluous load.
    return endpoint;
  }
  update_queue_->enqueue(request);

  while (*static_cast<bool*>(request->retval) && request->eptr == nullptr) {
    std::this_thread::yield();
  }
  if (request->eptr != nullptr) {
    std::rethrow_exception(request->eptr);
  }

  return endpoint;
}

void Manager::unloadWorker(std::string const& key) {
  if (workerExists(key)) {
    auto request =
      std::make_shared<UpdateCommand>(UpdateCommandType::Stop, key);
    update_queue_->enqueue(request);
  }
}

WorkerInfo* Manager::getWorker(std::string const& key) {
  if (active_workers_.find(key) != active_workers_.end()) {
    return active_workers_.at(key).get();
  }
  throw std::invalid_argument("Worker " + key + " not started");
}

bool Manager::workerExists(std::string const& key) {
  return active_workers_.find(key) != active_workers_.end();
}

bool Manager::workerReady(std::string const& key) {
  auto metadata = this->getWorkerMetadata(key);
  return metadata.isReady();
}

ModelMetadata Manager::getWorkerMetadata(std::string const& key) {
  auto* worker = this->getWorker(key);
  auto* foo = worker->workers_.begin()->second;
  return foo->getMetadata();
}

void Manager::addWorker(std::string const& key,
                        std::unique_ptr<WorkerInfo> worker_info_ptr) {
  active_workers_.insert(std::pair<std::string, std::unique_ptr<WorkerInfo> >(
    key, std::move(worker_info_ptr)));
}

void Manager::workerAllocate(std::string const& key, int num) {
  auto* worker = this->getWorker(key);
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

void Manager::shutdown() {
  auto request = std::make_shared<UpdateCommand>(UpdateCommandType::Shutdown);
  this->update_queue_->enqueue(request);
  if (this->update_thread_.joinable()) {
    this->update_thread_.join();
  }
}

void Manager::update_manager(UpdateCommandQueue* input_queue) {
  SPDLOG_LOGGER_DEBUG(this->logger_, "Starting the Manager update thread");
  setThreadName("manager");
  std::shared_ptr<UpdateCommand> request;
  bool run = true;
  while (run) {
    input_queue->wait_dequeue(request);
    SPDLOG_LOGGER_DEBUG(this->logger_, "Got request in Manager update thread");
    switch (request->cmd) {
      case UpdateCommandType::Duplicate:
        if (workerExists(request->key)) {
          auto* parameters = static_cast<RequestParameters*>(request->object);
          SPDLOG_LOGGER_DEBUG(this->logger_, "Adding a new worker");
          try {
            auto* worker_info = this->active_workers_.at(request->key).get();
            worker_info->addAndStartWorker(request->key, parameters);
          } catch (...) {
            request->eptr = std::current_exception();
          }
        }
        // fall through
      case UpdateCommandType::Create:
        if (!workerExists(request->key)) {
          auto* parameters = static_cast<RequestParameters*>(request->object);
          SPDLOG_LOGGER_DEBUG(this->logger_, "Creating a new worker");
          try {
            auto worker_info =
              std::make_unique<WorkerInfo>(request->key, parameters);
            this->addWorker(request->key, std::move(worker_info));
          } catch (...) {
            request->eptr = std::current_exception();
          }
        }
        *static_cast<bool*>(request->retval) = false;
        break;
      case UpdateCommandType::Shutdown:
        for (auto const& worker_info : this->active_workers_) {
          worker_info.second->shutdown();
        }
        this->active_workers_.clear();
        run = false;
        break;
      case UpdateCommandType::Stop:
        if (workerExists(request->key)) {
          auto* worker_info = active_workers_.at(request->key).get();
          worker_info->unload();
          if (worker_info->getGroupSize() == 0) {
            this->active_workers_.erase(request->key);
            auto hyphen_pos = request->key.find('-');
            if (hyphen_pos != std::string::npos) {
              request->key.erase(hyphen_pos);
            }
            this->active_worker_endpoints_.erase(request->key);
          }
        }
        break;
      case UpdateCommandType::Allocate:
        try {
          WorkerInfo* worker_info = active_workers_.at(request->key).get();
          auto num = *static_cast<int*>(request->object);
          if (!worker_info->inputSizeValid(num)) {
            SPDLOG_LOGGER_DEBUG(
              this->logger_,
              "Allocating more buffers for worker " + request->key);
            worker_info->allocate(num);
          }
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
      case UpdateCommandType::Load:
        try {
          auto parameters = *static_cast<RequestParameters*>(request->object);
          if (active_worker_endpoints_.find(request->key) ==
              active_worker_endpoints_.end()) {
            // this is a brand-new worker we haven't seen before
            std::map<RequestParameters, std::string> map;
            map.insert(std::make_pair(parameters, ""));
            active_worker_endpoints_.insert(
              std::make_pair(request->key, std::make_pair(0, map)));
            static_cast<std::string*>(request->retval)->assign(request->key);
          } else {
            auto endpoint = active_worker_endpoints_.at(request->key);
            auto index = endpoint.first;
            auto map = endpoint.second;
            if (map.find(parameters) == map.end()) {
              // we've seen this worker before but not with these parameters
              std::string url = "-" + std::to_string(index);
              map.insert(std::make_pair(parameters, url));
              index++;
              static_cast<std::string*>(request->retval)
                ->assign(request->key + url);
            } else {
              static_cast<std::string*>(request->retval)
                ->assign(request->key + map.at(parameters));
            }
          }
        } catch (...) {
          request->eptr = std::current_exception();
        }
        break;
    }
  }
  SPDLOG_LOGGER_DEBUG(this->logger_, "Ending update_thread");
}

}  // namespace proteus
