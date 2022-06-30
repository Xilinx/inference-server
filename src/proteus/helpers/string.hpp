#ifndef GUARD_PROTEUS_HELPERS_STRING
#define GUARD_PROTEUS_HELPERS_STRING

#include <algorithm>
#include <string>

namespace proteus {

/**
 * @brief Checks if a string ends with another string
 *
 * @param str full string
 * @param suffix ending to check
 * @return bool true if str ends with suffix
 */
inline bool endsWith(std::string_view str, std::string_view suffix) {
  return str.size() >= suffix.size() &&
         0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

inline std::string toLower(const std::string& str) {
  auto str_lower = str;
  std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str_lower;
}

inline void toLower(std::string* str) {
  std::transform(str->begin(), str->end(), str->begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

}  // namespace proteus

#endif  // GUARD_PROTEUS_HELPERS_STRING
