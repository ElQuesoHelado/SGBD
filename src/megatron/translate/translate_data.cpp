#include "megatron.hpp"
#include "serial/fixed_page.hpp"
#include "serial/slotted_page.hpp"

// Escribe sectores al disco
std::string Megatron::translate_data_page(serial::TableMetadata &table_metadata,
                                          std::vector<unsigned char> &page_bytes, size_t page_id) {
  std::span<unsigned char> page_data(page_bytes);

  std::vector<std::string> sectors;
  if (table_metadata.are_regs_fixed) {
    serial::FixedPage fixed_page(page_data, page_bytes);
    sectors = translate_fixed_page(table_metadata, fixed_page.page_header,
                                   fixed_page.fixed_data_header,
                                   page_bytes, page_id);

  } else {
    serial::SlottedPage slotted_page(page_data, page_bytes);

    sectors = translate_slotted_page(table_metadata, slotted_page.page_header,
                                     slotted_page.slotted_data_header, page_bytes, page_id);
  }

  while (sectors.size() > disk_manager->SECTORS_PER_BLOCK) {
    auto mod = sectors.size() % disk_manager->SECTORS_PER_BLOCK;
    auto sect_str = sectors.back();
    sectors.pop_back();

    sectors[mod] += sect_str;
  }

  size_t i{};
  std::string page_str{}, sect_str;
  for (auto &e : sectors) {
    sect_str = disk_manager->logic_sector_to_CHS(
                   disk_manager->free_block_map.get_ith_lba(
                       page_id, i)) +
               '\n' + e;
    page_str += sect_str;
    disk_manager->write_sector_txt(
        sect_str,
        disk_manager->free_block_map.get_ith_lba(page_id, i));
    i++;
  }

  return page_str;
}

// Es una pagina completa, implica varios sectores, el primero es afectado por el header
std::vector<std::string> Megatron::translate_fixed_page(
    serial::TableMetadata &table_metadata,
    serial::PageHeader &page_header,
    serial::FixedDataHeader &fixed_data_header,
    std::vector<unsigned char> &page_bytes, uint32_t curr_page_id) {

  // Primer sector tiene la metadata de pagina
  std::vector<std::string> sectors;
  sectors.emplace_back();

  int remm_sector_bytes =
      disk_manager->SECTOR_SIZE -
      serial::PageHeader::size() -
      fixed_data_header.size();

  // PageHeader
  std::string out_str = page_header.to_string() +
                        '/' + std::to_string(disk_manager->BLOCK_SIZE) + "\n";

  // FixedDataHeader
  out_str += fixed_data_header.to_string() + "\n";

  sectors.back() += out_str;

  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    if (remm_sector_bytes - (int)fixed_data_header.reg_size < 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;

      sectors.emplace_back();
    }

    if (fixed_data_header.free_register_bitmap->test(i)) {
      auto register_bytes =
          get_ith_register_bytes(table_metadata,
                                 page_header,
                                 fixed_data_header, page_bytes, i);
      auto register_values =
          deserialize_register(table_metadata, register_bytes);

      for (auto &v : register_values)
        sectors.back() += SQL_type_to_string(v) + " ";

      sectors.back() += '\n';
    } else { // Registro vacio/deleted
      sectors.back() += '\n';
    }

    remm_sector_bytes -= fixed_data_header.reg_size;
  }

  return sectors;
}

std::vector<std::string> Megatron::translate_slotted_page(
    serial::TableMetadata &table_metadata,
    serial::PageHeader &page_header,
    serial::SlottedDataHeader &slotted_data_header,
    std::vector<unsigned char> &page_bytes, uint32_t curr_page_id) {

  std::vector<std::string> sectors;
  sectors.emplace_back();

  int remm_sector_bytes =
      disk_manager->SECTOR_SIZE -
      serial::PageHeader::size() -
      slotted_data_header.size();

  // PageHeader
  std::string out_str =
      page_header.to_string() +
      '/' + std::to_string(disk_manager->BLOCK_SIZE) + "\n";

  // SlottedDataHeader
  sectors.back() += out_str +
                    slotted_data_header.to_string() +
                    "\n";

  for (size_t i{}; i < slotted_data_header.n_slots; ++i) {
    if (remm_sector_bytes - (int)slotted_data_header.slots[i].reg_size < 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;

      sectors.emplace_back();
    }

    if (slotted_data_header.slots[i].is_used) {
      auto register_bytes =
          get_ith_register_bytes(table_metadata,
                                 page_header,
                                 slotted_data_header, page_bytes, i);
      auto register_values =
          deserialize_register(table_metadata, register_bytes);

      for (auto &v : register_values)
        sectors.back() += SQL_type_to_string(v) + " ";

      sectors.back() += '\n';
    } else { // Registro vacio/deleted
      sectors.back() += '\n';
    }
    // Permite incluir
    remm_sector_bytes -= slotted_data_header.slots[i].reg_size;
  }

  return sectors;
}
