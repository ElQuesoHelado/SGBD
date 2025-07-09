#include "hlpr.hpp"
#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>

void Megatron::insert_hashed(
    serial::TableMetadata &table_metadata,
    size_t col_index, size_t page_id, size_t pos,
    std::vector<unsigned char> &register_bytes) {
  auto dir_page_id = get_root_page_id(table_metadata, col_index);

  if (dir_page_id == disk_manager->NULL_BLOCK) {
    std::cerr << "No se tiene un hash para esa columna" << std::endl;
    return;
  }

  // Se encuentra indice de bucket
  auto depth = get_depth(table_metadata, col_index);
  auto dir_index = hash_index(register_bytes, depth);

  while (dir_page_id != disk_manager->NULL_BLOCK) {
    auto &frame = buffer_manager->load_pin_page(page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;
    auto it = page_bytes.begin();

    auto page_header = serial::deserialize_page_header(it);
    auto fixed_data_header = serial::deserialize_fixed_data_header(it);

    // FIXME: Parecido a update nth

    dir_page_id = page_header.next_block_id;

    buffer_manager->free_unpin_page(page_id, false);
  }
}
