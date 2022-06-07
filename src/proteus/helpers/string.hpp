#ifndef GUARD_PROTEUS_HELPERS_STRING
#define GUARD_PROTEUS_HELPERS_STRING

#include <string>

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

#endif  // GUARD_PROTEUS_HELPERS_STRING
