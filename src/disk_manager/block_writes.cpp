#include "disk_manager.hpp"
#include <cstddef>
#include <cstdint>

void DiskManager::write_block(std::vector<unsigned char> &block_bytes, uint32_t block_id) {
  if (block_bytes.size() != BLOCK_SIZE && block_id >= free_block_map.blocks.size())
    throw std::runtime_error("Bloques y sectores a escribir no concuerdan");

  std::vector<std::vector<unsigned char>>
      vec_of_sectors = split_block_to_sectors(std::move(block_bytes));
  for (size_t i{}; i < vec_of_sectors.size(); ++i) {
    write_sector(vec_of_sectors[i], free_block_map.get_ith_lba(block_id, i));
  }
  free_block_map.set_block_used(block_id);
}

uint32_t DiskManager::write_block(std::vector<unsigned char> &block_bytes) {
  if (block_bytes.size() != BLOCK_SIZE)
    throw std::runtime_error("Bloques y sectores a escribir no concuerdan");

  auto free_block_id = get_free_block();

  write_block(block_bytes, free_block_id);

  return free_block_id;
}
