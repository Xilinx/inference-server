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
#include <prometheus/metric_family.h>    // for MetricFamily
#include <prometheus/registry.h>         // for Registry
#include <prometheus/serializer.h>       // for Serializer
#include <prometheus/summary.h>          // for Summary, BuildSummary, Summa...
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
  const std::unordered_map<MetricIDs, std::map<std::string, std::string>>&
    labels)
  : family_(
      prometheus::BuildCounter().Name(name).Help(help).Register(*registry)) {
  for (const auto& [id, label] : labels) {
    counters_.emplace(id, family_.Add(label));
  }
}

void CounterFamily::increment(MetricIDs id) {
  if (this->counters_.find(id) != this->counters_.end()) {
    auto& counter = this->counters_.at(id);
    counter.Increment();
  }
}

void CounterFamily::increment(MetricIDs id, size_t increment) {
  if (this->counters_.find(id) != this->counters_.end()) {
    auto& counter = this->counters_.at(id);
    counter.Increment(increment);
  }
}

GaugeFamily::GaugeFamily(
  const std::string& name, const std::string& help,
  prometheus::Registry* registry,
  const std::unordered_map<MetricIDs, std::map<std::string, std::string>>&
    labels)
  : family_(
      prometheus::BuildGauge().Name(name).Help(help).Register(*registry)) {
  for (const auto& [id, label] : labels) {
    gauges_.emplace(id, family_.Add(label));
  }
}

void GaugeFamily::set(MetricIDs id, double value) {
  if (this->gauges_.find(id) != this->gauges_.end()) {
    auto& gauge = this->gauges_.at(id);
    gauge.Set(value);
  }
}

Metrics::Metrics()
  :
#ifdef PROTEUS_ENABLE_LOGGING
    logger_(getLogger()),
#endif
    registry_(std::make_shared<prometheus::Registry>()),

    ingress_requests_total_(
      "proteus_requests_ingress_total",
      "Number of incoming requests to Proteus", registry_.get(),
      {{MetricIDs::kCounterCppNative, {{"api", "cpp"}, {"method", "native"}}},
       {MetricIDs::kCounterRestGet, {{"api", "rest"}, {"method", "GET"}}},
       {MetricIDs::kCounterRestPost, {{"api", "rest"}, {"method", "POST"}}}}),
    pipeline_ingress_total_(
      "proteus_pipeline_ingress_total",
      "Number of incoming requests at different pipeline stages",
      registry_.get(),
      {{MetricIDs::kCounterPipelineIngressBatcher, {{"stage", "batcher"}}},
       {MetricIDs::kCounterPipelineIngressWorker, {{"stage", "worker"}}}}),
    pipeline_egress_total_(
      "proteus_pipeline_egress_total",
      "Number of outgoing requests at different pipeline stages",
      registry_.get(),
      {{MetricIDs::kCounterPipelineEgressBatcher, {{"stage", "batcher"}}},
       {MetricIDs::kCounterPipelineEgressWorker, {{"stage", "worker"}}}}),
    bytes_transferred_("exposer_transferred_bytes_total",
                       "Transferred bytes to metrics services", registry_.get(),
                       {{MetricIDs::kCounterTransferredBytes, {}}}),
    num_scrapes_("exposer_scrapes_total",
                 "Number of times metrics were scraped", registry_.get(),
                 {{MetricIDs::kCounterMetricScrapes, {}}}),
    queue_sizes_total_("proteus_queue_sizes_total",
                       "Number of elements in the queues in Proteus",
                       registry_.get(),
                       {{MetricIDs::kGaugeQueuesBatcherInput,
                         {{"direction", "input"}, {"stage", "batcher"}}},
                        {MetricIDs::kGaugeQueuesBatcherOutput,
                         {{"direction", "output"}, {"stage", "batcher"}}},
                        {MetricIDs::kGaugeQueuesBufferInput,
                         {{"direction", "input"}, {"stage", "buffer"}}},
                        {MetricIDs::kGaugeQueuesBufferOutput,
                         {{"direction", "output"}, {"stage", "buffer"}}}}),
    req_latencies_(
      prometheus::BuildSummary()
        .Name("exposer_request_latencies")
        .Help("Latencies of serving scrape requests, in microseconds")
        .Register(*this->registry_)),
    req_latencies_counter_(
      // quantiles are represented by {quantile, error_margin}
      req_latencies_.Add(
        {}, prometheus::Summary::Quantiles{
              {0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}})) {  // NOLINT

  std::lock_guard<std::mutex> lock{this->collectables_mutex_};
  collectables_.push_back(this->registry_);

  this->serializer_ = std::make_unique<prometheus::TextSerializer>();
}

void Metrics::incrementCounter(MetricIDs id, size_t increment) {
  switch (id) {
    case MetricIDs::kCounterRestGet:
    case MetricIDs::kCounterRestPost:
    case MetricIDs::kCounterCppNative:
      this->ingress_requests_total_.increment(id);
      break;
    case MetricIDs::kCounterPipelineIngressBatcher:
    case MetricIDs::kCounterPipelineIngressWorker:
      this->pipeline_ingress_total_.increment(id);
      break;
    case MetricIDs::kCounterPipelineEgressBatcher:
    case MetricIDs::kCounterPipelineEgressWorker:
      this->pipeline_egress_total_.increment(id);
      break;
    case MetricIDs::kCounterTransferredBytes:
      this->bytes_transferred_.increment(id, increment);
      break;
    case MetricIDs::kCounterMetricScrapes:
      this->num_scrapes_.increment(id);
      break;
    default:
      break;
  }
}

void Metrics::setGauge(MetricIDs id, double value) {
  switch (id) {
    case MetricIDs::kGaugeQueuesBatcherInput:
    case MetricIDs::kGaugeQueuesBatcherOutput:
    case MetricIDs::kGaugeQueuesBufferInput:
    case MetricIDs::kGaugeQueuesBufferOutput:
      this->queue_sizes_total_.set(id, value);
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
  this->req_latencies_counter_.Observe(duration.count());

  this->bytes_transferred_.increment(MetricIDs::kCounterTransferredBytes,
                                     bodySize);
  this->num_scrapes_.increment(MetricIDs::kCounterMetricScrapes);

  return response;
}

}  // namespace proteus
