#pragma once

#include "types.hpp"
#include <cstddef>
#include <span>

struct SQL_type {
  SQL_type_ value;

  uint8_t type{};
  uint16_t size{};

  void deserialize(std::span<std::byte> data) {
  }

  void serialize();
  std::string to_string();
};
