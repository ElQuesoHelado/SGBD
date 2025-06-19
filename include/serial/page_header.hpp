#pragma once

#include "serial/generic.hpp"
#include <cstdint>
#include <vector>

namespace serial {

#pragma pack(push, 1)
struct PageHeader {
  uint32_t next_block_id{};
  uint32_t free_space{};
  uint16_t n_regs{};
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

inline PageHeader deserialize_page_header(std::vector<unsigned char> &bytes) {
  PageHeader header;
  auto it = bytes.begin();

  read_v(it, header);

  return header;
}

template <typename Iter>
inline PageHeader deserialize_page_header(Iter &in_it) {
  PageHeader header;

  read_v(in_it, header);

  return header;
}
} // namespace serial
