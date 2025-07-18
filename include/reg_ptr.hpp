#pragma once

#include <cstddef>
struct RegPtr {
  size_t page_id;
  size_t slot;

  RegPtr(size_t p, size_t s) : page_id(p), slot(s) {}

  bool operator==(const RegPtr &other) const {
    return page_id == other.page_id && slot == other.slot;
  }
};
