#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

/*
 * Marca bloques de forma vertical, de igual forma marca si estan siendo usados
 */
struct FreeBlockMap {
  std::vector<std::pair<bool, std::vector<uint32_t>>> blocks{};

  bool is_block_free(uint32_t block_id) {
    if (block_id >= blocks.size())
      throw std::runtime_error("block_id fuera de rango");

    return !blocks[block_id].first;
  }

  uint32_t get_ith_lba(uint32_t block_id, size_t ith_lba) {
    if (block_id >= blocks.size())
      throw std::runtime_error("block_id fuera de rango");

    return blocks[block_id].second[ith_lba];
  }

  void set_block_used(uint32_t block_id) {
    if (block_id >= blocks.size())
      throw std::runtime_error("block_id fuera de rango");

    blocks[block_id].first = true;
  }
};
