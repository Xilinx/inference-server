#ifndef GUARD_AMDINFER_CORE_VERSIONED_ENDPOINT
#define GUARD_AMDINFER_CORE_VERSIONED_ENDPOINT

#include <regex>
#include <string>

namespace amdinfer {

inline std::string getVersionedEndpoint(const std::string& model,
                                        const std::string& version) {
  if (version.empty()) {
    return model;
  }
  return model + "_" + version;
}

inline std::pair<std::string, std::string> splitVersionedEndpoint(
  const std::string& endpoint) {
  // match the last _<#> in the string
  static std::regex version_regex{R"(([\w-]+)(_)(\d+))"};
  // match[1] is the model, match[2] is _, and match[3] is the version
  std::smatch match;
  auto found = std::regex_search(endpoint, match, version_regex);
  if (!found) {
    return {std::string{endpoint}, ""};
  }

  auto model = std::string(match[1].first, match[1].second);
  auto version = std::string(match[3].first, match[3].second);

  return {model, version};
}

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_VERSIONED_ENDPOINT
