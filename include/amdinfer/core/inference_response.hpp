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
 * @brief Defines the objects used to hold inference responses
 */

#ifndef GUARD_AMDINFER_CORE_INFERENCE_RESPONSE
#define GUARD_AMDINFER_CORE_INFERENCE_RESPONSE

#include "amdinfer/build_options.hpp"    // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/parameters.hpp"  // for ParameterMap
#include "amdinfer/declarations.hpp"     // for InferenceResponseOutput

namespace amdinfer {

/**
 * @brief Creates an inference response object based on KServe's V2 spec that
 * is used to respond back to clients.
 */
class InferenceResponse {
 public:
  /// Constructs a new InferenceResponse object
  InferenceResponse();

  /// Constructs a new InferenceResponse error object
  explicit InferenceResponse(const std::string &error);

  /// Gets a vector of the requested output information
  [[nodiscard]] std::vector<InferenceResponseOutput> getOutputs() const;
  /**
   * @brief Adds an output tensor to the response
   *
   * @param output an output tensor
   */
  void addOutput(const InferenceResponseOutput &output);

  /// Gets the ID of the response
  std::string getID() const { return id_; }
  /// Sets the ID of the response
  void setID(const std::string &id);
  /// sets the model name of the response
  void setModel(const std::string &model);
  /// gets the model name of the response
  std::string getModel();

  /// Checks if this is an error response
  bool isError() const;
  /// Gets the error message if it exists. Defaults to an empty string
  std::string getError() const;

#ifdef AMDINFER_ENABLE_TRACING
  /**
   * @brief Sets the Context object to propagate tracing data on
   *
   * @param context
   */
  void setContext(StringMap &&context);
  /// Gets the response's context to use to initialize a new span
  const StringMap &getContext() const;
#endif

  /// Gets a pointer to the parameters associated with this response
  ParameterMap *getParameters() { return this->parameters_.get(); }

  /// Provides an implementation to print the class with std::cout to an ostream
  friend std::ostream &operator<<(std::ostream &os,
                                  InferenceResponse const &my_class);

 private:
  std::string model_;
  std::string id_;
  std::shared_ptr<ParameterMap> parameters_;
  std::vector<InferenceResponseOutput> outputs_;
  std::string error_msg_;
#ifdef AMDINFER_ENABLE_TRACING
  StringMap context_;
#endif
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_INFERENCE_RESPONSE
