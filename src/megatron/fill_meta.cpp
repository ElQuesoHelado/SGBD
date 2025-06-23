#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string_view>
#include <unordered_map>

/*
 * Mapa para asignar tamanios a tipos sql,
 * en caso CHAR su size depende de tamanio N
 * para VARCHAR es N + 1 byte de tamanio
 */

// Devuelve varchar/char(n)
// Size 0 se considera invalido
// TODO: trim espacios
size_t Megatron::char_size(std::string type) {
  size_t op_paren = type.find('('),
         clos_paren = type.find(')');

  // Que exista y al menos un numero en ()
  if (op_paren == std::string::npos || clos_paren == std::string::npos || clos_paren <= op_paren + 1)
    return 0;

  std::string size_str = type.substr(op_paren + 1, clos_paren - op_paren - 1);

  // tiene que ser numero
  if (!std::all_of(size_str.begin(), size_str.end(), [](unsigned char c) {
        return std::isdigit(c);
      }))
    return 0;
  ;

  return std::stoi(size_str);
}

/*
 * Crea una tabla nueva
 * @note crea en disco una nueva pagina, NO escribe tabla
 */
void Megatron::init_table_metadata(serial::TableMetadata &table_metadata,
                                   std::string name, uint32_t block_id,
                                   std::vector<std::pair<std::string, std::string>> &columns) {
  table_metadata.table_block_id = block_id;
  std::strncpy(table_metadata.name.data(), name.c_str(), table_metadata.name.size());

  table_metadata.n_cols = columns.size();
  table_metadata.max_reg_size = 0;

  table_metadata.columns.reserve(table_metadata.n_cols);
  for (auto &p : columns) {
    table_metadata.columns.emplace_back();

    auto &col_name = table_metadata.columns.back().name;
    auto &col_type = table_metadata.columns.back().type;
    auto &col_max_size = table_metadata.columns.back().max_size;

    std::strncpy(col_name.data(), p.first.c_str(), col_name.size());

    // Se determina el max size para una columna, nombre y tipos
    if (auto it = type_to_size.find(p.second); it != type_to_size.end()) {
      col_max_size = it->second;
      col_type = type_to_index.at(p.second);

    } else if (size_t n_chars = char_size(p.second); n_chars != 0) { // char/varchar(n)
      std::string type;
      if (p.second.compare(0, 5, "CHAR(") == 0)
        type = "CHAR";
      else { // TODO: Revisar varchar
        type = "VARCHAR";
        n_chars++; // Byte de tamanio
        table_metadata.are_regs_fixed = false;
      }

      col_max_size = n_chars;
      col_type = type_to_index.at(type);
      // std::strncpy(col_type.data(), type.c_str(), col_type.size());
    } else {
      std::cerr << "Tipo inexistente" << std::endl;
    }
    table_metadata.max_reg_size += col_max_size;
  }

  // Se crea una pagina inicial
  serial::FixedDataHeader fixed_data_header;
  serial::SlottedDataHeader slotted_data_header;
  serial::PageHeader page_header;

  // buffer_manager_ptr->get_block(size_t block_id);
  std::vector<unsigned char> page_bytes(disk_manager->BLOCK_SIZE);
  auto page_it = page_bytes.begin();

  if (table_metadata.are_regs_fixed) {
    init_fixed_data_header(table_metadata, fixed_data_header);
    init_page_header(page_header, fixed_data_header.free_bytes);

    serial::serialize_page_header(page_header, page_it);
    serial::serialize_fixed_block_header(fixed_data_header, page_it);

  } else {
    init_slotted_data_header(table_metadata, slotted_data_header);
    init_page_header(page_header, slotted_data_header.free_bytes);

    serial::serialize_page_header(page_header, page_it);
    serial::serialize_slotted_data_header(slotted_data_header, page_it);
  }

  // TODO: logica en buffer manager, deberia ser una dirty page mas del resto
  auto page_id = disk_manager->write_block(page_bytes);

  table_metadata.first_page_id = page_id;
  table_metadata.last_page_id = page_id;
}

void Megatron::init_page_header(serial::PageHeader &page_header, uint32_t initial_free_space) {
  page_header.next_block_id = disk_manager->NULL_BLOCK;
  page_header.free_space = initial_free_space;
  page_header.n_regs = 0;
}

void Megatron::init_fixed_data_header(serial::TableMetadata &table_metadata, serial::FixedDataHeader &fixed_data_header) {
  fixed_data_header.reg_size = table_metadata.max_reg_size;
  serial::calculate_max_n_regs(fixed_data_header, disk_manager->BLOCK_SIZE);
}

void Megatron::init_slotted_data_header(serial::TableMetadata &table_metadata, serial::SlottedDataHeader &slotted_data_header) {
  slotted_data_header.free_bytes = disk_manager->BLOCK_SIZE - serial::base_slotted_data_header_size(slotted_data_header) - sizeof(serial::PageHeader);
  slotted_data_header.free_space_offset = disk_manager->BLOCK_SIZE;
}
