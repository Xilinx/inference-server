#ifndef GUARD_AMDINFER_CORE_MIXINS
#define GUARD_AMDINFER_CORE_MIXINS

#include <cstddef>  // for std::byte, size_t

namespace amdinfer {

/**
 * @brief This class can be inherited from to implement serialization and
 * deserialization in a class.
 */
class Serializable {
 public:
  virtual ~Serializable() = default;

  /**
   * @brief Returns the size of the serialized data
   *
   * @return size_t
   */
  [[nodiscard]] virtual size_t serializeSize() const = 0;
  /**
   * @brief Serializes the object to the provided memory address. There should
   * be sufficient space to store the serialized object.
   *
   * @param data_out
   * @return std::byte* the updated address
   */
  virtual std::byte* serialize(std::byte* data_out) const = 0;
  /**
   * @brief Deserializes the data at the provided memory address to initialize
   * this object. If the memory cannot be deserialized, an exception is thrown.
   *
   * @param data_in a pointer to the serialized data for this object type
   * @return std::byte* the updated address
   */
  virtual const std::byte* deserialize(const std::byte* data_in) = 0;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MIXINS
