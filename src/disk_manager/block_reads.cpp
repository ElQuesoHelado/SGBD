#include "disk_manager.hpp"
#include <cstddef>
#include <cstdint>

void disk_manager::read_block(std::vector<unsigned char> &block,
                              uint32_t block_id) {
  if (block_id >= free_block_map.blocks.size())
    throw std::runtime_error("Bloques y sectores a leer no concuerdan");

  std::vector<std::vector<unsigned char>> vec_of_sectors;
  // vec_of_bytes.clear();
  vec_of_sectors.resize(SECTORS_PER_BLOCK);
  for (size_t i{}; i < SECTORS_PER_BLOCK; ++i) {
    read_sector(vec_of_sectors[i],
                free_block_map.get_ith_lba(block_id, i));
  }

  block = std::move(merge_sectors_to_block(vec_of_sectors));
}
