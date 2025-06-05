#pragma once

#include "generic.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <vector>

namespace serial {

struct Column {
  std::array<char, NAME_SIZE> name{};
  uint8_t type{};
  uint16_t max_size{};
};

/*
 * @struct TableMetadata
 * @brief Datos de tabla escritos en particion reservada
 *
 * Datos basicos de una relacion,
 * contiene un puntero a su primer archivo(cada uno apunta al siguiente)
 *
 * @note La busqueda se da linealmente en todos los bloques
 */
struct TableMetadata {
  uint32_t table_block_id{};
  std::array<char, NAME_SIZE> name{}; // 20 bytes
  size_t max_reg_size{};              // max por variante,
  uint8_t are_regs_fixed{1};          // en fijo  es igual que max

  uint32_t first_page_id{};
  uint32_t last_page_id{};
  u_int8_t n_cols{};
  std::vector<Column> columns{};
};

/*
 * @brief Deserializa metadata de un buffer(un bloque completo)
 */
inline TableMetadata deserialize_table_metadata(std::vector<unsigned char> &block) {
  TableMetadata metadata;
  auto it = block.begin();

  read_v(it, metadata.table_block_id);
  read_v(it, metadata.name);
  read_v(it, metadata.max_reg_size);
  read_v(it, metadata.are_regs_fixed);
  read_v(it, metadata.first_page_id);
  read_v(it, metadata.last_page_id);

  read_v(it, metadata.n_cols);

  metadata.columns.resize(metadata.n_cols);
  for (uint8_t i{}; i < metadata.n_cols; ++i) {
    read_v(it, metadata.columns[i].name);
    read_v(it, metadata.columns[i].type);
    read_v(it, metadata.columns[i].max_size);
  }

  return metadata;
}

/*
 * @brief Serializa metadata de una tabla
 * Se devuelve un bloque completo(esto calculado con sector_size * n_sectors_block)
 */
inline std::vector<unsigned char> serialize_table_metadata(
    TableMetadata &metadata, size_t block_size) {
  std::vector<unsigned char> bytes(block_size);

  auto bytes_it = bytes.begin();

  write_v(bytes_it, metadata.table_block_id);
  write_v(bytes_it, metadata.name);
  write_v(bytes_it, metadata.max_reg_size);
  write_v(bytes_it, metadata.are_regs_fixed);
  write_v(bytes_it, metadata.first_page_id);
  write_v(bytes_it, metadata.last_page_id);

  write_v(bytes_it, metadata.n_cols);

  for (auto &p : metadata.columns) {
    write_v(bytes_it, p.name);
    write_v(bytes_it, p.type);
    write_v(bytes_it, p.max_size);
  }

  return bytes;
}
//
/*
 * Funcion temporal para solucionar revisado de tamanios variables de metadatas
 */
// inline size_t get_table_metadata_size(std::vector<unsigned char> &block, size_t offset = 0) {
//   size_t size = sizeof(TableMetadata::name) + sizeof(TableMetadata::n_files) + sizeof(TableMetadata::file_size) +
//                 sizeof(TableMetadata::file_lba) + sizeof(TableMetadata::n_cols),
//          n_cols{};
//
//   // Bytes insuficientes para leer parte no variante
//   if (block.size() < offset + size)
//     return 0;
//
//   auto it = block.begin() + offset + size;
//
//   read(it, n_cols);
//
//   size += n_cols * sizeof(decltype(TableMetadata::columns)::value_type);
//
//   // Bytes infuficientes para leer columnas
//   if (block.size() < offset + size)
//     return 0;
//
//   return size;
// }

} // namespace serial
