#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include <cstdint>

uint32_t Megatron::get_insertable_page(std::vector<unsigned char> &page, uint32_t first_block_id, uint32_t reg_size) {
  uint32_t curr_block_id = first_block_id;

  while (curr_block_id != disk.NULL_BLOCK) {
    std::vector<unsigned char> block;
    disk.read_block(block, curr_block_id);

    auto page_header = serial::deserialize_page_header(block);
    if (page_header.free_space >= reg_size) {
      page = std::move(block);
      return curr_block_id;
    }
    curr_block_id = page_header.next_block_id;
  }

  return disk.NULL_BLOCK;
}
