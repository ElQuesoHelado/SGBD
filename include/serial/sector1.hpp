#pragma once

#include "serial/generic.hpp"
#include <cstddef>
#include <cstdint>

namespace serial {

/*
 * Representa las ubicaciones de todas las relaciones
 * en disco(sector reservado 1)
 * A cada tabla le corresponde un bloque para
 * guardado de metadata
 */
struct Sector_1 {
  uint8_t n_tables{};
  std::vector<uint32_t> table_block_ids{};
};

inline std::vector<unsigned char> serialize_sector1(const Sector_1 &metadata) {
  std::vector<unsigned char> bytes;

  bytes.reserve(sizeof(metadata.n_tables) + metadata.n_tables * sizeof(uint32_t));

  write_v(bytes, metadata.n_tables);

  for (const auto &block_id : metadata.table_block_ids) {
    write_v(bytes, block_id);
  }

  return bytes;
}

/*
 * TODO: tal vez lento
 */
inline Sector_1 deserialize_sector1(const std::vector<unsigned char> &bytes) {
  Sector_1 metadata;

  auto bytes_it = bytes.begin();

  read_v(bytes_it, metadata.n_tables);

  metadata.table_block_ids.resize(metadata.n_tables);

  // Un bloque a cada tabla
  for (size_t i{}; i < metadata.n_tables; ++i)
    read_v(bytes_it, metadata.table_block_ids[i]);

  return metadata;
}

} // namespace serial
