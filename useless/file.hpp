#pragma once
#include "generic.hpp"
#include "serial/fixed_block.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace serial {
/*
 * @struct Agrupacion de paginas, es escrito continuamente
 * @note
 * Primer bloque es metadata misma, los siguientes corresponen a registros
 * Su creacion depende de un TableMetadata
 * TODO: bitmap bloques libres / parcialmente libres / llenos
 * Primer bloque parcialmente lleno/vacio

 */
#pragma pack(push, 1)
struct FileHeader {
  uint32_t reg_size{}; // TODO: Deberia moverse a blocks, por ahora duplicado

  uint8_t block_type{}; // Los bloques podrian ser slotted/fijos con bitmap/indexados

  uint8_t block_header_size{};
  uint32_t block_size{}; // Size completo
  uint32_t n_blocks{};   // Calculado con size inicial de file
  uint32_t next_file_lba{};
  // Se busca linearmente un bloque libre primero para inserts
};
#pragma pack(pop)

inline std::vector<unsigned char> serialize_file(const FileHeader &file) {
  std::vector<unsigned char> buffer;

  write(buffer, file);

  return buffer;
}

inline FileHeader deserialize_file(std::vector<unsigned char> &block) {
  FileHeader file;

  auto it = block.begin();
  read(it, file);

  return file;
}

} // namespace serial
