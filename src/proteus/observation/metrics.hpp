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

#include "proteus/build_options.hpp"        // for PROTEUS_ENABLE_METRICS
#include "proteus/observation/logging.hpp"  // for LoggerPtr

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

/**
 * @brief Defines the IDs we can use to refer to different metrics that
 * Prometheus is tracking.
 *
 */
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

enum class MetricGaugeIDs {
  kQueuesBatcherInput,
  kQueuesBatcherOutput,
  kQueuesBufferInput,
  kQueuesBufferOutput,
};

enum class MetricSummaryIDs {
  kMetricLatency,
  kRequestLatency,
};

class CounterFamily {
 public:
  CounterFamily(
    const std::string& name, const std::string& help,
    prometheus::Registry* registry,
    const std::unordered_map<MetricCounterIDs,
                             std::map<std::string, std::string>>& labels);

  void increment(MetricCounterIDs id);
  void increment(MetricCounterIDs id, size_t increment);

 private:
  prometheus::Family<prometheus::Counter>& family_;
  std::unordered_map<MetricCounterIDs, prometheus::Counter&> counters_;
};

class GaugeFamily {
 public:
  GaugeFamily(const std::string& name, const std::string& help,
              prometheus::Registry* registry,
              const std::unordered_map<
                MetricGaugeIDs, std::map<std::string, std::string>>& labels);

  void set(MetricGaugeIDs id, double value);

 private:
  prometheus::Family<prometheus::Gauge>& family_;
  std::unordered_map<MetricGaugeIDs, prometheus::Gauge&> gauges_;
};

class SummaryFamily {
 public:
  SummaryFamily(
    const std::string& name, const std::string& help,
    prometheus::Registry* registry,
    const std::unordered_map<MetricSummaryIDs, prometheus::Summary::Quantiles>&
      quantiles);

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

  /// Gets a shared pointer to the registry of metrics that are being tracked.
  // std::shared_ptr<prometheus::Registry> getRegistry();
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

  void setGauge(MetricGaugeIDs id, double value);

  void observeSummary(MetricSummaryIDs id, double value);

 private:
  /// Construct a new Metrics object
  Metrics();
  /// Destroy the Metrics object
  ~Metrics() = default;

#ifdef PROTEUS_ENABLE_LOGGING
  LoggerPtr logger_;
#endif

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
