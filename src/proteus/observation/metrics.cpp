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
 * @brief Implements what metrics are defined and how they're are gathered
 */

#include "proteus/observation/metrics.hpp"

#include <prometheus/collectable.h>      // for Collectable
#include <prometheus/counter.h>          // for Builder, Counter, BuildCounter
#include <prometheus/family.h>           // for Family
#include <prometheus/gauge.h>            // for Gauge, BuildGauge
#include <prometheus/metric_family.h>    // for MetricFamily
#include <prometheus/registry.h>         // for Registry
#include <prometheus/serializer.h>       // for Serializer
#include <prometheus/text_serializer.h>  // for TextSerializer

#include <chrono>    // for microseconds, duration, dura...
#include <iterator>  // for move_iterator, make_move_ite...
#include <memory>    // for weak_ptr, allocator, shared_ptr
#include <string>    // for string
#include <vector>    // for vector

namespace proteus {

CounterFamily::CounterFamily(
  const std::string& name, const std::string& help,
  prometheus::Registry* registry,
  const std::unordered_map<MetricCounterIDs,
                           std::map<std::string, std::string>>& labels)
  : family_(
      prometheus::BuildCounter().Name(name).Help(help).Register(*registry)) {
  for (const auto& [id, label] : labels) {
    counters_.emplace(id, family_.Add(label));
  }
}

void CounterFamily::increment(MetricCounterIDs id) {
  if (this->counters_.find(id) != this->counters_.end()) {
    auto& counter = this->counters_.at(id);
    counter.Increment();
  }
}

void CounterFamily::increment(MetricCounterIDs id, size_t increment) {
  if (this->counters_.find(id) != this->counters_.end()) {
    auto& counter = this->counters_.at(id);
    counter.Increment(increment);
  }
}

GaugeFamily::GaugeFamily(
  const std::string& name, const std::string& help,
  prometheus::Registry* registry,
  const std::unordered_map<MetricGaugeIDs, std::map<std::string, std::string>>&
    labels)
  : family_(
      prometheus::BuildGauge().Name(name).Help(help).Register(*registry)) {
  for (const auto& [id, label] : labels) {
    gauges_.emplace(id, family_.Add(label));
  }
}

void GaugeFamily::set(MetricGaugeIDs id, double value) {
  if (this->gauges_.find(id) != this->gauges_.end()) {
    auto& gauge = this->gauges_.at(id);
    gauge.Set(value);
  }
}

SummaryFamily::SummaryFamily(
  const std::string& name, const std::string& help,
  prometheus::Registry* registry,
  const std::unordered_map<MetricSummaryIDs, prometheus::Summary::Quantiles>&
    quantiles)
  : family_(
      prometheus::BuildSummary().Name(name).Help(help).Register(*registry)) {
  for (const auto& [id, quantile] : quantiles) {
    summaries_.emplace(id, family_.Add({}, quantile));
  }
}

void SummaryFamily::observe(MetricSummaryIDs id, double value) {
  if (this->summaries_.find(id) != this->summaries_.end()) {
    auto& summary = this->summaries_.at(id);
    summary.Observe(value);
  }
}

Metrics::Metrics()
  : registry_(std::make_shared<prometheus::Registry>()),

    ingress_requests_total_(
      "proteus_requests_ingress_total",
      "Number of incoming requests to Proteus", registry_.get(),
      {{MetricCounterIDs::kCppNative, {{"api", "cpp"}, {"method", "native"}}},
       {MetricCounterIDs::kRestGet, {{"api", "rest"}, {"method", "GET"}}},
       {MetricCounterIDs::kRestPost, {{"api", "rest"}, {"method", "POST"}}}}),
    pipeline_ingress_total_(
      "proteus_pipeline_ingress_total",
      "Number of incoming requests at different pipeline stages",
      registry_.get(),
      {{MetricCounterIDs::kPipelineIngressBatcher, {{"stage", "batcher"}}},
       {MetricCounterIDs::kPipelineIngressWorker, {{"stage", "worker"}}}}),
    pipeline_egress_total_(
      "proteus_pipeline_egress_total",
      "Number of outgoing requests at different pipeline stages",
      registry_.get(),
      {{MetricCounterIDs::kPipelineEgressBatcher, {{"stage", "batcher"}}},
       {MetricCounterIDs::kPipelineEgressWorker, {{"stage", "worker"}}}}),
    bytes_transferred_("exposer_transferred_bytes_total",
                       "Transferred bytes to metrics services", registry_.get(),
                       {{MetricCounterIDs::kTransferredBytes, {}}}),
    num_scrapes_("exposer_scrapes_total",
                 "Number of times metrics were scraped", registry_.get(),
                 {{MetricCounterIDs::kMetricScrapes, {}}}),
    queue_sizes_total_("proteus_queue_sizes_total",
                       "Number of elements in the queues in Proteus",
                       registry_.get(),
                       {{MetricGaugeIDs::kQueuesBatcherInput,
                         {{"direction", "input"}, {"stage", "batcher"}}},
                        {MetricGaugeIDs::kQueuesBatcherOutput,
                         {{"direction", "output"}, {"stage", "batcher"}}},
                        {MetricGaugeIDs::kQueuesBufferInput,
                         {{"direction", "input"}, {"stage", "buffer"}}},
                        {MetricGaugeIDs::kQueuesBufferOutput,
                         {{"direction", "output"}, {"stage", "buffer"}}}}),
    metric_latency_("exposer_request_latencies",
                    "Latencies of serving scrape requests, in microseconds",
                    registry_.get(),
                    {{MetricSummaryIDs::kMetricLatency,
                      prometheus::Summary::Quantiles{
                        {0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}}}}),
    request_latency_("proteus_request_latency",
                     "Latencies of serving requests, in microseconds",
                     registry_.get(),
                     {{MetricSummaryIDs::kRequestLatency,
                       prometheus::Summary::Quantiles{
                         {0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}}}}) {
  std::lock_guard<std::mutex> lock{this->collectables_mutex_};
  collectables_.push_back(this->registry_);

  this->serializer_ = std::make_unique<prometheus::TextSerializer>();
}

void Metrics::incrementCounter(MetricCounterIDs id, size_t increment) {
  switch (id) {
    case MetricCounterIDs::kRestGet:
    case MetricCounterIDs::kRestPost:
    case MetricCounterIDs::kCppNative:
      this->ingress_requests_total_.increment(id);
      break;
    case MetricCounterIDs::kPipelineIngressBatcher:
    case MetricCounterIDs::kPipelineIngressWorker:
      this->pipeline_ingress_total_.increment(id);
      break;
    case MetricCounterIDs::kPipelineEgressBatcher:
    case MetricCounterIDs::kPipelineEgressWorker:
      this->pipeline_egress_total_.increment(id);
      break;
    case MetricCounterIDs::kTransferredBytes:
      this->bytes_transferred_.increment(id, increment);
      break;
    case MetricCounterIDs::kMetricScrapes:
      this->num_scrapes_.increment(id);
      break;
    default:
      break;
  }
}

void Metrics::setGauge(MetricGaugeIDs id, double value) {
  switch (id) {
    case MetricGaugeIDs::kQueuesBatcherInput:
    case MetricGaugeIDs::kQueuesBatcherOutput:
    case MetricGaugeIDs::kQueuesBufferInput:
    case MetricGaugeIDs::kQueuesBufferOutput:
      this->queue_sizes_total_.set(id, value);
      break;
    default:
      break;
  }
}

void Metrics::observeSummary(MetricSummaryIDs id, double value) {
  switch (id) {
    case MetricSummaryIDs::kMetricLatency:
      this->metric_latency_.observe(id, value);
      break;
    case MetricSummaryIDs::kRequestLatency:
      this->request_latency_.observe(id, value);
      break;
    default:
      break;
  }
}

std::string Metrics::getMetrics() {
  auto start_time_of_request = std::chrono::steady_clock::now();

  std::vector<prometheus::MetricFamily> metrics;

  {
    std::lock_guard<std::mutex> lock{this->collectables_mutex_};

    for (auto&& wcollectable : collectables_) {
      auto collectable = wcollectable.lock();
      if (!collectable) {
        continue;
      }

      auto&& my_metrics = collectable->Collect();
      metrics.insert(metrics.end(), std::make_move_iterator(my_metrics.begin()),
                     std::make_move_iterator(my_metrics.end()));
    }
  }

  std::string response = serializer_->Serialize(metrics);
  auto bodySize = response.length();

  auto stop_time_of_request = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
    stop_time_of_request - start_time_of_request);
  this->observeSummary(MetricSummaryIDs::kMetricLatency, duration.count());

  this->bytes_transferred_.increment(MetricCounterIDs::kTransferredBytes,
                                     bodySize);
  this->num_scrapes_.increment(MetricCounterIDs::kMetricScrapes);

  return response;
}

}  // namespace proteus
