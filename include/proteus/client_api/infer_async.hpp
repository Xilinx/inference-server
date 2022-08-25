#ifndef GUARD_PROTEUS_CLIENT_API_INFER_ASYNC
#define GUARD_PROTEUS_CLIENT_API_INFER_ASYNC

#include <string>
#include <vector>

#include "proteus/clients/client.hpp"
#include "proteus/core/predict_api.hpp"

namespace proteus {

std::vector<InferenceResponse> infer_async_ordered(
  Client* client, const std::string& model,
  const std::vector<InferenceRequest>& requests);
std::vector<InferenceResponse> infer_async_ordered_batched(
  Client* client, const std::string& model,
  const std::vector<InferenceRequest>& requests, size_t batch_size);

}  // namespace proteus

#endif  // GUARD_PROTEUS_CLIENT_API_INFER_ASYNC
