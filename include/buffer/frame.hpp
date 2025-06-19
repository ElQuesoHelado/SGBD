#pragma once

#include <cstddef>
#include <vector>

struct Frame {
  std::vector<unsigned char> page_bytes;
  size_t page_id;
  bool dirty = false;

  Frame(size_t id, std::vector<unsigned char> data)
      : page_bytes(std::move(data)), page_id(id) {}
};
