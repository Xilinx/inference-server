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
 * @brief Defines how the shared mutable state is managed
 */

#ifndef GUARD_AMDINFER_CORE_MANAGER
#define GUARD_AMDINFER_CORE_MANAGER

#include <exception>      // for exception_ptr
#include <map>            // for map
#include <memory>         // for allocator, unique_ptr
#include <string>         // for string
#include <thread>         // for thread
#include <unordered_map>  // for unordered_map
#include <utility>        // for move, pair
#include <vector>         // for vector

#include "amdinfer/build_options.hpp"        // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/core/predict_api.hpp"     // for RequestParameters
#include "amdinfer/core/worker_info.hpp"     // for WorkerInfo
#include "amdinfer/observation/logging.hpp"  // for LoggerPtr
#include "amdinfer/util/queue.hpp"           // for BlockingConcurrentQueue

// IWYU pragma: no_forward_declare amdinfer::RequestParameters
// IWYU pragma: no_forward_declare amdinfer::WorkerInfo

namespace amdinfer {

/**
 * @brief IDs used to specify commands to update the Manager
 *
 */
enum class UpdateCommandType {
  Shutdown,
  Allocate,
  Add,
  Delete,
  Ready,
};

/**
 * @brief Commands sent to update the Manager consist of an ID, a key
 * value (string), an integer, and a pointer to an exception so if the update
 * fails for some reason, this information is communicated back to the requester
 */
struct UpdateCommand {
  /// Constructor for UpdateCommand
  explicit UpdateCommand(UpdateCommandType cmd, std::string key = "",
                         void* object = nullptr, void* retval = nullptr)
    : cmd(cmd), key(std::move(key)), object(object), retval(retval) {}
  /// the command ID
  UpdateCommandType cmd;
  /// a string key that a command can make use of. Usually identifies the worker
  std::string key;
  /// pointer to an arbitrary object
  void* object;
  /// pointer to a caller-allocated variable to hold the return value
  void* retval = nullptr;
  /**
   * @brief The caller making a request through the update mechanism should
   * catch this exception which is thrown if the requested update fails so the
   * caller is not waiting endlessly.
   */
  std::exception_ptr eptr = nullptr;
};
using UpdateCommandQueue = BlockingQueue<std::shared_ptr<UpdateCommand>>;

/**
 * @brief The Manager holds all the state information about a running
 * amdinfer-server. Read access to the state is thread-safe but all
 * modifications are handled through a separate update thread to preserve
 * consistency. It is a singleton instance and the base code is taken from
 * https://stackoverflow.com/a/1008289.
 */
class Manager {
 public:
  /// Get the singleton Manager instance
  static Manager& getInstance() {
    // Guaranteed to be destroyed. Instantiated on first use.
    static Manager instance;
    return instance;
  }

  Manager(Manager const&) = delete;             ///< Copy constructor
  Manager& operator=(const Manager&) = delete;  ///< Copy assignment constructor
  Manager(Manager&& other) = delete;            ///< Move constructor
  Manager& operator=(Manager&& other) =
    delete;  ///< Move assignment constructor

  std::string loadWorker(std::string const& key, RequestParameters parameters);
  void unloadWorker(std::string const& key);

  /**
   * @brief Get the WorkerInfo object associated with the given key. If the
   * worker does not exist, returns nullptr
   *
   * @param key name of the worker
   * @return WorkerInfo*
   */
  WorkerInfo* getWorker(std::string const& key) const;

  std::vector<std::string> getWorkerEndpoints();

  bool workerReady(const std::string& key) const;
  ModelMetadata getWorkerMetadata(const std::string& key) const;

  /**
   * @brief Request that a worker support a request with num inputs. This means
   * that the worker must allocate enough buffers to have at least num buffers.
   *
   * @param key name of the worker to make the request to
   * @param num the minimum number of buffers the worker should have after
   * allocation
   */
  void workerAllocate(std::string const& key, int num);

  /**
   * @brief Initialize the Manager. This is called automatically by the
   * constructor and may be called again after shutdown() to restart. If already
   * initialized, this does nothing.
   */
  void init();
  /**
   * @brief Stop the Manager. This should be called prior to ending the server.
   *
   */
  void shutdown();

 private:
  /// Construct a new Manager object
  Manager();
  /// Destroy the Manager object
  ~Manager();

  /**
   * @brief The Endpoints class is a helper class to bundle up all the worker
   * state data structures and operations within the Manager. Its methods should
   * be called from the updateManager.
   */
  class Endpoints {
   public:
    std::string load(const std::string& worker, RequestParameters* parameters);
    void unload(const std::string& endpoint);

    bool exists(const std::string& endpoint);
    WorkerInfo* get(const std::string& endpoint) const;
    std::vector<std::string> list() const;

    std::string add(const std::string& worker, RequestParameters parameters);

    void shutdown();

   private:
    // worker -> map[parameters -> endpoint]
    std::unordered_map<std::string, std::map<RequestParameters, std::string>>
      worker_endpoints_;
    // worker -> index
    std::unordered_map<std::string, int> worker_indices_;
    // endpoint -> parameters
    std::unordered_map<std::string, RequestParameters> worker_parameters_;
    // endpoint -> Worker_Info*
    std::unordered_map<std::string, std::unique_ptr<WorkerInfo>> workers_;
  };

  /// instantiation of the Endpoints class for maintaining state
  Endpoints endpoints_;
  /// A queue used to sequentially order changes to the Manager state
  std::unique_ptr<UpdateCommandQueue> update_queue_;
  std::thread update_thread_;
#ifdef AMDINFER_ENABLE_LOGGING
  Logger logger_{Loggers::Server};
#endif

  /**
   * @brief This method is started as a separate thread when the Manager is
   * constructed. It monitors a queue which contains commands that modify the
   * shared state. This queue serializes these requests and ensures
   * consistency
   *
   * @param input_queue queue where update requests will arrive
   */
  void updateManager(UpdateCommandQueue* input_queue);
};

}  // namespace amdinfer
#endif  // GUARD_AMDINFER_CORE_MANAGER
