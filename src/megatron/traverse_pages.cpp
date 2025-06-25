#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include <cstdint>

uint32_t Megatron::get_insertable_page(uint32_t first_block_id, uint32_t reg_size) {
  uint32_t curr_block_id = first_block_id;

  while (curr_block_id != disk_manager->NULL_BLOCK) {
    // disk.read_block(block, curr_block_id);

    auto &frame = buffer_manager->load_pin_page(curr_block_id);
    std::vector<unsigned char> &block = frame.page_bytes;

    auto page_header = serial::deserialize_page_header(block);
    if (page_header.free_space >= reg_size) {
      // page = std::move(block);
      buffer_manager->free_unpin_page(curr_block_id, 0);
      return curr_block_id;
    }

    buffer_manager->free_unpin_page(curr_block_id, 0);

    curr_block_id = page_header.next_block_id;
  }

  return disk_manager->NULL_BLOCK;
}
