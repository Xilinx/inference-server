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

#ifndef GUARD_AMDINFER_CORE_ENDPOINTS
#define GUARD_AMDINFER_CORE_ENDPOINTS

#include <exception>      // for exception_ptr
#include <map>            // for map
#include <memory>         // for allocator, uniq...
#include <string>         // for string
#include <thread>         // for thread
#include <unordered_map>  // for unordered_map
#include <utility>        // for move
#include <vector>         // for vector

#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE...
#include "amdinfer/core/memory_pool/pool.hpp"  // for MemoryPool
#include "amdinfer/core/model_metadata.hpp"    // for ModelMetadata
#include "amdinfer/core/parameters.hpp"        // for ParameterMap
#include "amdinfer/observation/logging.hpp"    // for Logger, Loggers
#include "amdinfer/util/queue.hpp"             // for BlockingQueue

namespace amdinfer {

class RequestContainer;
class WorkerInfo;

/**
 * @brief IDs used to specify commands to update the Manager
 *
 */
enum class UpdateCommandType {
  Load,
  Unload,
  Exists,
  Ready,
  List,
  Metadata,
  Shutdown,
};

/**
 * @brief Commands sent to update the Manager consist of an ID, a key
 * value (string), an integer, and a pointer to an exception so if the update
 * fails for some reason, this information is communicated back to the requester
 */
struct UpdateCommand {
  UpdateCommand(UpdateCommandType cmd, std::string key = "",
                void* object = nullptr, void* retval = nullptr)
    : cmd(cmd), key(std::move(key)), object(object), retval(retval) {}
  /// The command ID
  UpdateCommandType cmd;
  /// A string key that a command can make use of. Usually identifies the worker
  std::string key;
  /// Pointer to an arbitrary object
  void* object;
  /// Pointer to a caller-allocated variable to hold the return value
  void* retval = nullptr;
  /**
   * @brief The caller making a request through the update mechanism should
   * catch this exception which is thrown if the requested update fails so the
   * caller is not waiting endlessly.
   */
  std::exception_ptr eptr = nullptr;
};
using UpdateCommandQueue = BlockingQueue<std::shared_ptr<UpdateCommand>>;

class Endpoints {
 public:
  Endpoints();
  ~Endpoints();

  std::string load(const std::string& worker, ParameterMap parameters);
  void unload(const std::string& endpoint);

  void infer(const std::string& endpoint,
             std::unique_ptr<RequestContainer> request) const;

  bool exists(const std::string& endpoint);
  // WorkerInfo* get(const std::string& endpoint);
  bool ready(const std::string& endpoint);

  std::vector<std::string> list();
  ModelMetadata metadata(const std::string& endpoint);

  const MemoryPool* getPool() const;

  void shutdown();

 private:
  // worker -> map[parameters -> endpoint]
  std::unordered_map<std::string, std::map<ParameterMap, std::string>>
    worker_endpoints_;
  // worker -> index
  std::unordered_map<std::string, int> worker_indices_;
  // endpoint -> parameters
  std::unordered_map<std::string, ParameterMap> worker_parameters_;
  // endpoint -> Worker_Info*
  std::unordered_map<std::string, std::unique_ptr<WorkerInfo>> workers_;
  /// A queue used to sequentially order changes to the Manager state
  UpdateCommandQueue update_queue_;
  std::thread update_thread_;
  MemoryPool pool_;
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
  std::string insertWorker(const std::string& worker,
                           const ParameterMap& parameters);

  std::string unsafeLoad(const std::string& worker, ParameterMap* parameters);
  void unsafeUnload(const std::string& endpoint);

  bool unsafeExists(const std::string& endpoint) const;
  WorkerInfo* unsafeGet(const std::string& endpoint) const;
  // bool unsafeReady(const std::string& endpoint) const;

  void unsafeList(std::vector<std::string>* list) const;
  ModelMetadata unsafeMetadata(const std::string& endpoint) const;

  void unsafeShutdown();
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_ENDPOINTS
