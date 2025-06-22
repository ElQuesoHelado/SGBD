#pragma once

#include <cstddef>
#include <list>
#include <memory>
#include <vector>

struct Frame {
  std::vector<unsigned char> page_bytes;
  size_t page_id;
  bool dirty = false;

  Frame(size_t id, std::vector<unsigned char> data)
      : page_bytes(std::move(data)), page_id(id) {}
};

struct BufferFrame {
  std::unique_ptr<Frame> frame;
  std::list<size_t>::iterator lru_it;
  int pin_count = 0;
  bool reference_bit = false; // TODO: clock
};
