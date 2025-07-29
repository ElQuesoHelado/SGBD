#include "megatron.hpp"

uint32_t Megatron::get_insertable_page_id(uint32_t first_page_id, uint32_t reg_size) {
  uint32_t curr_block_id = first_page_id;

  while (curr_block_id != disk_manager->NULL_BLOCK) {
    // disk.read_block(block, curr_block_id);

    auto &frame = buffer_manager->load_pin_page(curr_block_id);
    std::span<unsigned char> page_data(frame.page_bytes);

    serial::PageHeader page_header(page_data);

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
