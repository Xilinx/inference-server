#ifndef GUARD_PROTEUS_CORE_MIXINS
#define GUARD_PROTEUS_CORE_MIXINS

#include <cstddef>  // for std::byte, size_t

namespace proteus {

class Serializable {
 public:
  virtual ~Serializable() = default;

  virtual size_t serializeSize() const = 0;
  virtual void serialize(std::byte* data_out) const = 0;
  virtual void deserialize(const std::byte* data_in) = 0;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_MIXINS
