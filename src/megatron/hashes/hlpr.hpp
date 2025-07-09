#include "megatron.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>

inline std::vector<unsigned char> serialize_pointer(size_t block_id) {
  std::vector<unsigned char> bytes;
  bytes.reserve(sizeof(block_id));

  write_v(bytes, block_id);

  return bytes;
}

inline std::vector<unsigned char> serialize_pointer_position(size_t block_id, size_t pos) {
  std::vector<unsigned char> bytes;
  bytes.reserve(sizeof(block_id) + sizeof(pos));

  write_v(bytes, block_id);
  write_v(bytes, pos);

  return bytes;
}

// Encuentra hash en base a "d" bits menos significativos
// Necesariamente no podemos exceder de 32 bits de profundidad(size maximo de size_t)
inline size_t hash_index(std::vector<unsigned char> &bytes, size_t d) {
  size_t hash_value = 0;
  size_t bytes_to_copy = std::min(bytes.size(), sizeof(size_t));

  for (size_t i = 0; i < bytes_to_copy; ++i) {
    hash_value = (hash_value << 8) | bytes[i];
  }

  uint64_t mask = (d >= 32) ? ~0UL : (1UL << d) - 1;

  return hash_value & mask;
}
