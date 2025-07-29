#pragma once

#include "serial/generic.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

namespace serial {

#pragma pack(push, 1)
struct PageHeader {
  uint32_t &next_block_id;
  uint32_t &free_space;
  uint16_t &n_regs;

  static size_t size() {
    return sizeof(next_block_id) + sizeof(free_space) + sizeof(n_regs);
  }

  std::string to_string() {
    return "Next_page_id: " + std::to_string(next_block_id) + " " +
           "N_registers: " + std::to_string(n_regs) + " " +
           "Free_space: " + std::to_string(free_space);
  }

  PageHeader(std::span<unsigned char> &data) : next_block_id(*reinterpret_cast<uint32_t *>(data.data())),
                                               free_space(*reinterpret_cast<uint32_t *>(data.data() + 4)),
                                               n_regs(*reinterpret_cast<uint16_t *>(data.data() + 8)) {
    data = data.subspan(10); // Avanza el span
  }
};
#pragma pack(pop)

inline std::vector<unsigned char> serialize_page_header(const PageHeader &header) {
  std::vector<unsigned char> bytes;
  bytes.reserve(sizeof(PageHeader));

  write_v(bytes, header);

  return bytes;
}

template <typename Iter>
inline void serialize_page_header(const PageHeader &header, Iter &out_it) {
  write_v(out_it, header);
}

// inline PageHeader deserialize_page_header(std::vector<unsigned char> &bytes) {
//   PageHeader header;
//   auto it = bytes.begin();
//
//   read_v(it, header);
//
//   return header;
// }
//
// template <typename Iter>
// inline PageHeader deserialize_page_header(Iter &in_it) {
//   PageHeader header;
//
//   read_v(in_it, header);
//
//   return header;
// }
} // namespace serial
