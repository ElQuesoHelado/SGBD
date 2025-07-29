#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct RegPtr {
  uint32_t page_id;
  uint16_t slot;

  RegPtr(uint32_t p, uint16_t s) : page_id(p), slot(s) {}

  bool operator==(const RegPtr &other) const {
    return page_id == other.page_id && slot == other.slot;
  }
};
#pragma pack(pop)
