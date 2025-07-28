#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/fixed_page.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "serial/slotted_page.hpp"
#include "serial/table.hpp"
#include <cstddef>
#include <cstdint>
#include <utility>

// Implica modificar page y metadata de tabla
// Se inserta al final
// @notes Escribe tanto un page_header nuevo y un Data_header(depende de fixed o slotted)
uint32_t Megatron::add_new_page_to_table(serial::TableMetadata &table_metadata) {
  // Actualiza ultima pagina
  auto last_page_id = table_metadata.last_page_id;

  auto &last_frame = buffer_manager->load_pin_page(last_page_id);
  // std::vector<unsigned char> &last_page_bytes = last_frame.page_bytes;
  std::span<unsigned char> last_page_data(last_frame.page_bytes);

  serial::PageHeader page_header(last_page_data);

  if (page_header.next_block_id != disk_manager->NULL_BLOCK)
    throw std::runtime_error("Ultima pagina de tabla no tiene puntero a null");

  // Se crea nueva pagina de acuerdo a formato de table_metadata
  auto new_page_id = create_page(table_metadata);

  // Se actualiza puntero a next de ultima pagina y ptr a last page de tabla
  page_header.next_block_id = new_page_id;
  table_metadata.last_page_id = new_page_id;

  // Escribe tabla con nuevo puntero a pagina nueva(la ultima)
  auto &table_frame = buffer_manager->load_pin_page(table_metadata.table_block_id);
  auto &table_block_bytes = table_frame.page_bytes;
  table_block_bytes = serial::serialize_table_metadata(table_metadata, disk_manager->BLOCK_SIZE);

  buffer_manager->free_unpin_page(last_page_id, true);
  buffer_manager->free_unpin_page(table_metadata.table_block_id, true);

  return new_page_id;
}

/*
 * Se crea una pagina basada en una tabla
 * Escribe en disco,
 * @return block_id de pagina nueva
 */
uint32_t Megatron::create_page(serial::TableMetadata &table_metadata) {
  auto &new_frame = buffer_manager->get_load_free_frame();
  uint32_t free_block_id = new_frame.page_id;

  std::vector<unsigned char> &page_bytes = new_frame.page_bytes;
  std::span<unsigned char> page_data(page_bytes);

  // auto write_it = page_bytes.begin();

  // Se concatena page_header + data_header + fill
  if (table_metadata.are_regs_fixed) {
    auto [free_bytes, max_n_regs] =
        serial::calc_free_bytes_max_regs(disk_manager->BLOCK_SIZE,
                                         table_metadata.max_reg_size);

    serial::FixedPage fixed_page(
        page_data, page_bytes, free_bytes,
        table_metadata.max_reg_size, max_n_regs);

    // init_fixed_data_header(table_metadata, fixed_data_header);
    init_page_header(
        fixed_page.page_header,
        fixed_page.fixed_data_header.free_bytes);
  } else {
    serial::SlottedPage slotted_page(page_data, page_bytes);
    init_slotted_data_header(
        table_metadata,
        slotted_page.slotted_data_header);
    init_page_header(
        slotted_page.page_header,
        slotted_page.slotted_data_header.free_bytes);
  }

  buffer_manager->free_unpin_page(free_block_id, true);

  return free_block_id;
}

std::vector<std::pair<size_t, size_t>> Megatron::get_used_pages(serial::TableMetadata &table_metadata) {
  std::vector<std::pair<size_t, size_t>> res;

  uint32_t curr_block_id = table_metadata.first_page_id;

  while (curr_block_id != disk_manager->NULL_BLOCK) {
    auto &frame = buffer_manager->load_pin_page(curr_block_id);
    std::span<unsigned char> last_page_data(frame.page_bytes);

    serial::PageHeader page_header(last_page_data);

    res.emplace_back(curr_block_id, page_header.free_space);

    buffer_manager->free_unpin_page(curr_block_id, 0);

    curr_block_id = page_header.next_block_id;
  }

  return res;
}
