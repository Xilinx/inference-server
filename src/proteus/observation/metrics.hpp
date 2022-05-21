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
 * @brief Defines what metrics are defined and how they're are gathered
 */

#ifndef GUARD_PROTEUS_OBSERVATION_METRICS
#define GUARD_PROTEUS_OBSERVATION_METRICS

#include <prometheus/serializer.h>  // for Serializer
#include <prometheus/summary.h>     // for Summary, BuildSummary, Summa...

#include <cstddef>        // for size_t
#include <map>            // for map
#include <memory>         // for weak_ptr, shared_ptr, uni...
#include <mutex>          // for mutex
#include <string>         // for string
#include <unordered_map>  // for unordered_map
#include <vector>         // for vector

#include "proteus/build_options.hpp"  // for PROTEUS_ENABLE_METRICS

#ifdef PROTEUS_ENABLE_METRICS

namespace prometheus {
class Collectable;
class Counter;
class Gauge;
template <class T>
class Family;
class Registry;
}  // namespace prometheus

namespace proteus {

/// Defines the IDs of the counters tracked in Proteus
enum class MetricCounterIDs {
  kRestGet,
  kRestPost,
  kCppNative,
  kPipelineIngressBatcher,
  kPipelineIngressWorker,
  kPipelineEgressBatcher,
  kPipelineEgressWorker,
  kTransferredBytes,
  kMetricScrapes,
};

/// Defines the IDs of the gauges tracked in Proteus
enum class MetricGaugeIDs {
  kQueuesBatcherInput,
  kQueuesBatcherOutput,
  kQueuesBufferInput,
  kQueuesBufferOutput,
};

/// Defines the IDs of the summaries tracked in Proteus
enum class MetricSummaryIDs {
  kMetricLatency,
  kRequestLatency,
};

/**
 * @brief The CounterFamily class stores the counters tracked in Proteus and
 * provides methods to increment them using an ID.
 *
 */
class CounterFamily {
 public:
  /**
   * @brief Construct a new CounterFamily object
   *
   * @param name name of the counter
   * @param help help message for the counter
   * @param registry
   * @param labels map of IDs to counter labels
   */
  CounterFamily(
    const std::string& name, const std::string& help,
    prometheus::Registry* registry,
    const std::unordered_map<MetricCounterIDs,
                             std::map<std::string, std::string>>& labels);

  /// Increment the named counter by 1
  void increment(MetricCounterIDs id);
  /// Increment the named counter by increment
  void increment(MetricCounterIDs id, size_t increment);

 private:
  prometheus::Family<prometheus::Counter>& family_;
  std::unordered_map<MetricCounterIDs, prometheus::Counter&> counters_;
};

/**
 * @brief The GaugeFamily class stores the gauges tracked in Proteus and
 * provides methods to set them using an ID.
 *
 */
class GaugeFamily {
 public:
  /**
   * @brief Construct a new GaugeFamily object
   *
   * @param name name of the gauge
   * @param help help message for the gauge
   * @param registry
   * @param labels map of IDs to gauge labels
   */
  GaugeFamily(const std::string& name, const std::string& help,
              prometheus::Registry* registry,
              const std::unordered_map<
                MetricGaugeIDs, std::map<std::string, std::string>>& labels);

  /// Set the named gauge to a particular value
  void set(MetricGaugeIDs id, double value);

 private:
  prometheus::Family<prometheus::Gauge>& family_;
  std::unordered_map<MetricGaugeIDs, prometheus::Gauge&> gauges_;
};

/**
 * @brief The SummaryFamily class stores the summaries tracked in Proteus and
 * provides methods to record events.
 *
 */
class SummaryFamily {
 public:
  /**
   * @brief Construct a new SummaryFamily object
   *
   * @param name name of the summary
   * @param help help message of the summary
   * @param registry
   * @param quantiles map of IDs to quantiles to compute
   */
  SummaryFamily(
    const std::string& name, const std::string& help,
    prometheus::Registry* registry,
    const std::unordered_map<MetricSummaryIDs, prometheus::Summary::Quantiles>&
      quantiles);

  /// Record an event for a particular summary
  void observe(MetricSummaryIDs id, double value);

 private:
  prometheus::Family<prometheus::Summary>& family_;
  std::unordered_map<MetricSummaryIDs, prometheus::Summary&> summaries_;
};

/**
 * @brief The Metrics class exposes thread-safe methods for clients to update
 * metrics when events of interest occur. It also defines the body of the
 * request handler to return the serialized metric data to the client.
 */
class Metrics {
 public:
  /// Get the singleton Metrics instance
  static Metrics& getInstance() {
    // Guaranteed to be destroyed. Instantiated on first use.
    static Metrics instance;
    return instance;
  }
  Metrics(Metrics const&) = delete;             ///< Copy constructor
  Metrics& operator=(const Metrics&) = delete;  ///< Copy assignment constructor
  Metrics(Metrics&& other) = delete;            ///< Move constructor
  Metrics& operator=(Metrics&& other) =
    delete;  ///< Move assignment constructor

  /**
   * @brief Returns the collected metrics as a serialized string. This logic was
   * influenced by the examples included in prometheus-cpp pull (handler.cc).
   *
   * @return std::string
   */
  std::string getMetrics();
  /**
   * @brief Increment one named counter
   *
   * @param id counter to increment
   */
  void incrementCounter(MetricCounterIDs id, size_t increment = 1);

  /**
   * @brief Set one named gauge
   *
   * @param id gauge to set
   * @param value value to set the gauge to
   */
  void setGauge(MetricGaugeIDs id, double value);

  /**
   * @brief Record one event in a summary
   *
   * @param id summary to make the observation
   * @param value value to record
   */
  void observeSummary(MetricSummaryIDs id, double value);

 private:
  /// Construct a new Metrics object
  Metrics();
  /// Destroy the Metrics object
  ~Metrics() = default;

  std::shared_ptr<prometheus::Registry> registry_;
  std::unique_ptr<prometheus::Serializer> serializer_;
  std::vector<std::weak_ptr<prometheus::Collectable>> collectables_;
  std::mutex collectables_mutex_;

  CounterFamily ingress_requests_total_;
  CounterFamily pipeline_ingress_total_;
  CounterFamily pipeline_egress_total_;
  CounterFamily bytes_transferred_;
  CounterFamily num_scrapes_;
  GaugeFamily queue_sizes_total_;
  SummaryFamily metric_latency_;
  SummaryFamily request_latency_;
};

}  // namespace proteus

#endif

#endif  // GUARD_PROTEUS_OBSERVATION_METRICS
