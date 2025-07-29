#pragma once

#include "serial/generic.hpp"
#include <cstdint>

struct DirectoryPage {
  uint16_t global_depth{};
  std::vector<uint32_t> bucket_ptrs{};

  uint32_t page_id{}, null_page_id{};
  size_t capacity{};

  DirectoryPage(std::vector<unsigned char> &bytes, uint32_t directory_id,
                uint32_t null_page_id, size_t capacity)
      : page_id(directory_id), null_page_id(null_page_id), capacity(capacity) {
    deserialize(bytes, capacity);
  }

  DirectoryPage(uint32_t directory_id, uint32_t null_page_id, size_t capacity)
      : page_id(directory_id), null_page_id(null_page_id), capacity(capacity) {
    bucket_ptrs.resize(capacity, null_page_id);
  }

  template <typename Iter>
  void serialize(Iter &out_it) {
    write_v(out_it, global_depth);

    for (auto &b : bucket_ptrs)
      write_v(out_it, b);
  }

  void deserialize(
      std::vector<unsigned char> &bytes, uint32_t capacity) {
    auto it = bytes.begin();

    read_v(it, global_depth);

    bucket_ptrs.resize(capacity);
    for (auto &b : bucket_ptrs)
      read_v(it, b);
  }
};
