#pragma once

#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace serial {
/*
 * @struct Estructura para metadata de bloques interno de file donde
 * registros son de size fijos
 * @note El bitset tiene tamanio de la max cantidad de registros guardables
 */
struct FixedDataHeader {
  // Respecto a bloque completo, se resta tamanio mismo de header
  uint32_t free_bytes{};
  uint32_t reg_size{};
  uint16_t max_n_regs{};
  boost::dynamic_bitset<unsigned char>
      free_register_bitmap;
};

inline size_t calculate_fixed_data_header_size(const FixedDataHeader &header) {
  // if (header.max_n_regs != header.free_register_bitmap.size()) {
  //   std::cerr << "Corrupted FixedDataHeader" << std::endl;
  //   return 0;
  // }

  return sizeof(header.free_bytes) +
         sizeof(header.reg_size) +
         sizeof(header.max_n_regs) +
         (header.max_n_regs + CHAR_BIT - 1) / CHAR_BIT;
}

/*
 * Calcula todo menos reg_size, que es interno
 * Se toma en cuenta el espacio tomado por el page_header
 * @note Asume que ya se dio un valor inicial a reg_size
 */
inline void calculate_max_n_regs(FixedDataHeader &header, size_t page_size) {
  const uint32_t static_header_size = sizeof(header.free_bytes) +
                                      sizeof(header.reg_size) +
                                      sizeof(header.max_n_regs);
  if (header.reg_size == 0)
    throw std::runtime_error("reg_size no seteado para fill de otros campos");

  size_t bitmap_size = 1, page_header_size = sizeof(PageHeader);
  size_t n_regs = 0, new_bitmap_size = 0;

  while (true) {
    uint32_t free_space = page_size - static_header_size - bitmap_size - page_header_size;
    n_regs = free_space / header.reg_size;
    new_bitmap_size = (n_regs + CHAR_BIT - 1) / CHAR_BIT;

    if (new_bitmap_size == bitmap_size)
      break;

    bitmap_size = new_bitmap_size;
  }

  size_t usable_for_regs = page_size - static_header_size - bitmap_size - page_header_size;

  header.free_bytes = static_cast<uint32_t>(usable_for_regs);
  header.max_n_regs = static_cast<uint16_t>(n_regs);
  header.free_register_bitmap.resize(bitmap_size * CHAR_BIT);

  assert(header.free_bytes == n_regs * header.reg_size ||
         header.free_bytes >= (n_regs * header.reg_size && header.free_bytes < ((n_regs + 1) * header.reg_size)));
}

// Encuentra la posicion en bitmap que se encuentra libre
inline size_t find_free_reg_pos(const FixedDataHeader &header) {
  // if (header.max_n_regs != header.free_register_bitmap.size() || header.reg_size > header.free_bytes) {
  //   std::cerr << "FixedDataHeader invalido, sin capacidad suficiente" << std::endl;
  //   return header.max_n_regs;
  // }

  size_t ith_reg{};
  while (ith_reg < header.max_n_regs && header.free_register_bitmap[ith_reg])
    ith_reg++;

  return ith_reg;
}

// Calcula offset en bytes hacia un n registro
// @note Se considera size de page_header
inline size_t calculate_reg_offset(const FixedDataHeader &header, size_t nth_reg) {
  size_t offset = calculate_fixed_data_header_size(header) + sizeof(PageHeader);

  if (nth_reg >= header.max_n_regs) {
    throw std::runtime_error("FixedDataHeader invalido, sin capacidad suficiente");
    return 0;
  }

  offset += header.reg_size * nth_reg;
  return offset;
}

inline std::vector<unsigned char> serialize_fixed_data_header(const FixedDataHeader &header) {
  size_t size = calculate_fixed_data_header_size(header);

  std::vector<unsigned char> bytes;
  bytes.reserve(size);

  write_v(bytes, header.free_bytes);
  write_v(bytes, header.reg_size);
  write_v(bytes, header.max_n_regs);

  boost::to_block_range(header.free_register_bitmap, std::back_inserter(bytes));

  return bytes;
}

template <typename Iter>
inline void serialize_fixed_block_header(const FixedDataHeader &header, Iter &out_it) {
  write_v(out_it, header.free_bytes);
  write_v(out_it, header.reg_size);
  write_v(out_it, header.max_n_regs);

  size_t bitset_byte_size = (header.max_n_regs + CHAR_BIT - 1) / CHAR_BIT;

  boost::to_block_range(header.free_register_bitmap, out_it);
  std::advance(out_it, bitset_byte_size);
}

// Por el hecho de siempre estar despues de un PageHeader, se "salta" el tamanio del page_header del buffer
inline FixedDataHeader deserialize_fixed_data_header(std::vector<unsigned char> &page_bytes) {
  FixedDataHeader header;
  auto it = page_bytes.begin() + sizeof(PageHeader);

  read_v(it, header.free_bytes);
  read_v(it, header.reg_size);
  read_v(it, header.max_n_regs);

  size_t bitset_byte_size = (header.max_n_regs + CHAR_BIT - 1) / CHAR_BIT;

  // DeSerializacion de bitmap
  header.free_register_bitmap.resize(header.max_n_regs);
  boost::from_block_range(it, it + bitset_byte_size, header.free_register_bitmap);
  it += bitset_byte_size;

  return header;
}

template <typename Iter>
inline FixedDataHeader deserialize_fixed_data_header(Iter &in_it) {
  FixedDataHeader header;

  read_v(in_it, header.free_bytes);
  read_v(in_it, header.reg_size);
  read_v(in_it, header.max_n_regs);

  size_t bitset_byte_size = (header.max_n_regs + CHAR_BIT - 1) / CHAR_BIT;

  // DeSerializacion de bitmap
  header.free_register_bitmap.resize(header.max_n_regs);
  boost::from_block_range(in_it, std::next(in_it, bitset_byte_size), header.free_register_bitmap);
  std::advance(in_it, bitset_byte_size);

  return header;
}

} // namespace serial
