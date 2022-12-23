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
 * @brief Implements what metrics are defined and how they're are gathered
 */

#include "amdinfer/observation/metrics.hpp"

#include <prometheus/collectable.h>      // for Collectable
#include <prometheus/counter.h>          // for Builder, Counter, BuildCounter
#include <prometheus/family.h>           // for Family
#include <prometheus/gauge.h>            // for Gauge, BuildGauge
#include <prometheus/metric_family.h>    // for MetricFamily
#include <prometheus/serializer.h>       // for Serializer
#include <prometheus/text_serializer.h>  // for TextSerializer

#include <chrono>    // for microseconds, duration, dura...
#include <iterator>  // for move_iterator, make_move_ite...
#include <memory>    // for weak_ptr, allocator, shared_ptr
#include <string>    // for string
#include <vector>    // for vector

#include "prometheus/summary.h"  // for CKMSQuantiles, CKMSQuantiles...

namespace amdinfer {

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
    counter.Increment(static_cast<double>(increment));
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

// the arguments are percentile and error
// NOLINTNEXTLINE(cert-err58-cpp)
const prometheus::detail::CKMSQuantiles::Quantile kPercentile50{0.5, 0.05};
// NOLINTNEXTLINE(cert-err58-cpp)
const prometheus::detail::CKMSQuantiles::Quantile kPercentile90{0.9, 0.01};
// NOLINTNEXTLINE(cert-err58-cpp)
const prometheus::detail::CKMSQuantiles::Quantile kPercentile99{0.99, 0.001};

Metrics::Metrics()
  : ingress_requests_total_(
      "amdinfer_requests_ingress_total",
      "Number of incoming requests to amdinfer-server", registry_.get(),
      {{MetricCounterIDs::CppNative, {{"api", "cpp"}, {"method", "native"}}},
       {MetricCounterIDs::RestGet, {{"api", "rest"}, {"method", "GET"}}},
       {MetricCounterIDs::RestPost, {{"api", "rest"}, {"method", "POST"}}}}),
    pipeline_ingress_total_(
      "amdinfer_pipeline_ingress_total",
      "Number of incoming requests at different pipeline stages",
      registry_.get(),
      {{MetricCounterIDs::PipelineIngressBatcher, {{"stage", "batcher"}}},
       {MetricCounterIDs::PipelineIngressWorker, {{"stage", "worker"}}}}),
    pipeline_egress_total_(
      "amdinfer_pipeline_egress_total",
      "Number of outgoing requests at different pipeline stages",
      registry_.get(),
      {{MetricCounterIDs::PipelineEgressBatcher, {{"stage", "batcher"}}},
       {MetricCounterIDs::PipelineEgressWorker, {{"stage", "worker"}}}}),
    bytes_transferred_("exposer_transferred_bytes_total",
                       "Transferred bytes to metrics services", registry_.get(),
                       {{MetricCounterIDs::TransferredBytes, {}}}),
    num_scrapes_("exposer_scrapes_total",
                 "Number of times metrics were scraped", registry_.get(),
                 {{MetricCounterIDs::MetricScrapes, {}}}),
    queue_sizes_total_("amdinfer_queue_sizes_total",
                       "Number of elements in the queues in amdinfer-server",
                       registry_.get(),
                       {{MetricGaugeIDs::QueuesBatcherInput,
                         {{"direction", "input"}, {"stage", "batcher"}}},
                        {MetricGaugeIDs::QueuesBatcherOutput,
                         {{"direction", "output"}, {"stage", "batcher"}}},
                        {MetricGaugeIDs::QueuesBufferInput,
                         {{"direction", "input"}, {"stage", "buffer"}}},
                        {MetricGaugeIDs::QueuesBufferOutput,
                         {{"direction", "output"}, {"stage", "buffer"}}}}),
    metric_latency_("exposer_request_latencies",
                    "Latencies of serving scrape requests, in microseconds",
                    registry_.get(),
                    {{MetricSummaryIDs::MetricLatency,
                      prometheus::Summary::Quantiles{
                        kPercentile50, kPercentile90, kPercentile99}}}),
    request_latency_("amdinfer_request_latency",
                     "Latencies of serving requests, in microseconds",
                     registry_.get(),
                     {{MetricSummaryIDs::RequestLatency,
                       prometheus::Summary::Quantiles{
                         kPercentile50, kPercentile90, kPercentile99}}}) {
  std::lock_guard lock{this->collectables_mutex_};
  collectables_.push_back(this->registry_);

  this->serializer_ = std::make_unique<prometheus::TextSerializer>();
}

void Metrics::incrementCounter(MetricCounterIDs id, size_t increment) {
  switch (id) {
    case MetricCounterIDs::RestGet:
    case MetricCounterIDs::RestPost:
    case MetricCounterIDs::CppNative:
      this->ingress_requests_total_.increment(id);
      break;
    case MetricCounterIDs::PipelineIngressBatcher:
    case MetricCounterIDs::PipelineIngressWorker:
      this->pipeline_ingress_total_.increment(id);
      break;
    case MetricCounterIDs::PipelineEgressBatcher:
    case MetricCounterIDs::PipelineEgressWorker:
      this->pipeline_egress_total_.increment(id);
      break;
    case MetricCounterIDs::TransferredBytes:
      this->bytes_transferred_.increment(id, increment);
      break;
    case MetricCounterIDs::MetricScrapes:
      this->num_scrapes_.increment(id);
      break;
    default:
      break;
  }
}

void Metrics::setGauge(MetricGaugeIDs id, double value) {
  switch (id) {
    case MetricGaugeIDs::QueuesBatcherInput:
    case MetricGaugeIDs::QueuesBatcherOutput:
    case MetricGaugeIDs::QueuesBufferInput:
    case MetricGaugeIDs::QueuesBufferOutput:
      this->queue_sizes_total_.set(id, value);
      break;
    default:
      break;
  }
}

void Metrics::observeSummary(MetricSummaryIDs id, double value) {
  switch (id) {
    case MetricSummaryIDs::MetricLatency:
      this->metric_latency_.observe(id, value);
      break;
    case MetricSummaryIDs::RequestLatency:
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
  auto body_size = response.length();

  auto stop_time_of_request = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
    stop_time_of_request - start_time_of_request);
  this->observeSummary(MetricSummaryIDs::MetricLatency, duration.count());

  this->bytes_transferred_.increment(MetricCounterIDs::TransferredBytes,
                                     body_size);
  this->num_scrapes_.increment(MetricCounterIDs::MetricScrapes);

  return response;
}

}  // namespace amdinfer
