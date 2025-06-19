#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>

/*
 * Realiza el cargado de datos del disco
 * Considera campos solo para output y condiciones
 * ex: se quiere dos campos pero la condicion depende de otro no visualizado
 */
// void Megatron::select(Relation &relation, std::vector<std::string> &fields, std::vector<std::string> &conditions) {
void Megatron::select(std::string &table_name, std::string &col_name, std::string &condition) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

  // Se parsea column index y condicion a SQL_type
  size_t col_index{table_metadata.n_cols};
  SQL_type cond_val;

  for (size_t i{}; i < table_metadata.columns.size(); ++i) {
    if (col_name == array_to_string_view(table_metadata.columns[i].name)) {
      col_index = i;
      cond_val = string_to_sql_type(condition, table_metadata.columns[i].type, table_metadata.columns[i].max_size);
    }
  }

  if (table_metadata.are_regs_fixed)
    select_fixed(table_metadata, col_index, cond_val);
  else
    select_slotted(table_metadata, col_index, cond_val);
}

void Megatron::select_fixed(serial::TableMetadata &table_metadata,
                            size_t col_index, SQL_type &cond_val) {
  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id, n_regs{};
  while (curr_page_id != disk.NULL_BLOCK) {
    // disk.read_block(page_bytes, curr_page_id);
    auto frame = buffer_manager_ptr->get_block(curr_page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;

    auto page_bytes_it = page_bytes.begin();

    // Se saca metadata relevante
    serial::PageHeader page_header;
    serial::FixedDataHeader fixed_data_header;
    serial::SlottedDataHeader slotted_data_header;

    page_header = serial::deserialize_page_header(page_bytes_it);

    fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);
    for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {

      if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
        auto register_bytes = get_ith_register_bytes(table_metadata, page_header, fixed_data_header, page_bytes, i);
        auto register_values = deserialize_register(table_metadata, register_bytes);

        // Si hay condicion
        if (col_index < table_metadata.n_cols && register_values[col_index] != cond_val)
          continue;

        n_regs++;
        for (auto &v : register_values)
          std::cout << SQL_type_to_string(v) << " | ";

        std::cout << std::endl;
      }
    }

    curr_page_id = page_header.next_block_id;
  }
  std::cout << "Numero de registros: " << n_regs << std::endl;
}

void Megatron::select_slotted(serial::TableMetadata &table_metadata, size_t col_index, SQL_type &cond_val) {
  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id, n_regs{};
  while (curr_page_id != disk.NULL_BLOCK) {

    auto frame = buffer_manager_ptr->get_block(curr_page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;

    auto page_bytes_it = page_bytes.begin();

    // Se saca metadata relevante
    serial::PageHeader page_header;
    serial::FixedDataHeader fixed_data_header;
    serial::SlottedDataHeader slotted_data_header;

    page_header = serial::deserialize_page_header(page_bytes_it);

    slotted_data_header = serial::deserialize_slotted_data_header(page_bytes_it);
    for (size_t i{}; i < slotted_data_header.n_slots; ++i) {
      if (slotted_data_header.slots[i].is_used) { // Registro existe
        auto register_bytes = get_ith_register_bytes(table_metadata, page_header,
                                                     slotted_data_header, page_bytes, i);
        auto register_values = deserialize_register(table_metadata, register_bytes);

        // Si hay condicion
        if (col_index < table_metadata.n_cols && register_values[col_index] != cond_val)
          continue;

        n_regs++;
        for (auto &v : register_values)
          std::cout << SQL_type_to_string(v) << " | ";

        std::cout << std::endl;
      }
    }

    curr_page_id = page_header.next_block_id;
  }
  std::cout << "Numero de registros: " << n_regs << std::endl;
}
