#pragma once

#include <cstddef>
#include <vector>

struct Frame {
  std::vector<unsigned char> page_bytes; // Datos en bruto
  size_t page_id{};                      // FIXME: deberia ser NULL??
  bool dirty{};
};
