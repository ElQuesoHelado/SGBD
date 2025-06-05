#pragma once

#include "generic.hpp"
#include <cstdint>

namespace serial {

/*
 * Representa metadata esencial de un disco
 */
#pragma pack(push, 1)
struct Sector_0 {
  uint16_t surfaces{};
  uint16_t tracks_per_surf{};
  uint16_t sectors_per_track{};
  uint16_t sector_size{};
  uint16_t sectors_per_block{};
};
#pragma pack(pop)

inline std::vector<unsigned char> serialize_sector0(const Sector_0 &metadata) {
  std::vector<unsigned char> bytes;
  bytes.reserve(sizeof(Sector_0));

  write_v(bytes, metadata);

  return bytes;
}

inline Sector_0 deserialize_sector0(const std::vector<unsigned char> &bytes) {
  Sector_0 metadata;
  auto it = bytes.begin();

  read_v(it, metadata);

  return metadata;
}

} // namespace serial
