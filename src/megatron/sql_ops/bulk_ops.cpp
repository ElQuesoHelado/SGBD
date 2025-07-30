#include "megatron.hpp"
#include "result_set.hpp"
#include "serial/fixed_page.hpp"
#include "serial/slotted_page.hpp"
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <print>

void Megatron::load_CSV(std::string path, std::string table_name, size_t n_regs) {
  serial::TableMetadata table_metadata;

  // Metadata invalida, no existe tabla
  if (!std::filesystem::exists(path) ||
      !search_table(table_name, table_metadata)) {
    std::cerr << "Tabla o csv no existe" << std::endl;
    return;
  }

  rapidcsv::Document csv_file(
      path,
      rapidcsv::LabelParams(-1, -1));

  size_t regs_to_insert = (n_regs == 0) ? csv_file.GetRowCount() : n_regs;

  if (regs_to_insert <= 200) {
    for (size_t i{}; i < regs_to_insert; ++i) {
      auto row = csv_file.GetRow<std::string>(i);
      insert(table_metadata, row);
    }
  } else {
    bulk_insert(table_metadata, csv_file, regs_to_insert);
  }
}

void Megatron::bulk_insert(
    serial::TableMetadata &table_metadata,
    rapidcsv::Document &csv_file, size_t regs_to_insert) {
  auto result_set =
      (table_metadata.are_regs_fixed)
          ? bulk_insert_fixed(table_metadata, csv_file, regs_to_insert)
          : bulk_insert_slotted(table_metadata, csv_file, regs_to_insert);

  insert_set_hash(table_metadata, result_set);
  insert_set_index(table_metadata, result_set);
}

ResultSet Megatron::bulk_insert_fixed(
    serial::TableMetadata &table_metadata,
    rapidcsv::Document &csv_file, size_t regs_to_insert) {
  ResultSet result_set;
  result_set.add_columns(table_metadata.columns);

  auto page_id = table_metadata.last_page_id;

  size_t i{};
  while (regs_to_insert > 0) {
    auto &frame = buffer_manager->load_pin_page(page_id);
    std::span<unsigned char> insert_page_data(frame.page_bytes);
    serial::FixedPage fixed_page(insert_page_data,
                                 frame.page_bytes);

    while (fixed_page.fixed_data_header.free_bytes >=
               table_metadata.max_reg_size &&
           regs_to_insert > 0) {
      auto reg_row = csv_file.GetRow<std::string>(i);
      auto register_entry =
          str_values_to_register_entry(table_metadata, reg_row);
      auto register_bytes =
          serialize_register(table_metadata, register_entry.values);

      auto free_reg_pos =
          fixed_page.insert_register_bytes(register_bytes);

      regs_to_insert--;
      i++;

      register_entry.reg_ptr.page_id = page_id;
      register_entry.reg_ptr.slot = free_reg_pos;

      result_set.add_register(std::move(register_entry));
    }

    buffer_manager->free_unpin_page(page_id, true);

    if (regs_to_insert > 0) {
      page_id = add_new_page_to_table(table_metadata);
    }
  }

  return result_set;
}

ResultSet Megatron::bulk_insert_slotted(
    serial::TableMetadata &table_metadata,
    rapidcsv::Document &csv_file, size_t regs_to_insert) {
  ResultSet result_set;
  auto page_id = table_metadata.last_page_id;

  size_t i{};
  while (regs_to_insert > 0) {
    auto &frame = buffer_manager->load_pin_page(page_id);
    std::span<unsigned char> insert_page_data(frame.page_bytes);
    serial::SlottedPage slotted_page(insert_page_data,
                                     frame.page_bytes);

    while (regs_to_insert > 0) {
      auto reg_row = csv_file.GetRow<std::string>(i);
      auto register_entry =
          str_values_to_register_entry(table_metadata, reg_row);
      auto register_bytes =
          serialize_register(table_metadata, register_entry.values);

      if (slotted_page.slotted_data_header.free_bytes <
          register_bytes.size() + sizeof(serial::Slot)) {
        break;
      }

      auto free_reg_pos =
          slotted_page.insert_register_bytes(register_bytes);
      regs_to_insert--;
      i++;

      register_entry.reg_ptr.page_id = page_id;
      register_entry.reg_ptr.slot = free_reg_pos;

      result_set.add_register(std::move(register_entry));
    }

    buffer_manager->free_unpin_page(page_id, true);

    if (regs_to_insert > 0) {
      page_id = add_new_page_to_table(table_metadata);
    }
  }

  return result_set;
}
