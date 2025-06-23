#include "disk_manager.hpp"
#include "megatron.hpp"
#include "serial/generic.hpp"
#include "serial/sector1.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

bool Megatron::create_table(std::string name, std::vector<std::pair<std::string, std::string>> &columns) {
  if (name.empty() || columns.empty()) {
    std::cerr << "Nombre o columnas inválidas" << std::endl;
    return 0;
  }

  serial::TableMetadata table_metadata;

  if (search_table(name, table_metadata)) {
    std::cerr << "Tabla " << name << "ya existe en schema" << std::endl;
    return 0;
  }

  // Se carga todo sector 1
  std::vector<unsigned char> sector1_bytes;
  disk_manager->read_sector(sector1_bytes, 1);

  auto sector1_meta = serial::deserialize_sector1(sector1_bytes);

  // Bloque donde se va a almacenar tabla
  sector1_meta.n_tables++;
  sector1_meta.table_block_ids.push_back(disk_manager->reserve_free_block());

  init_table_metadata(table_metadata, name, sector1_meta.table_block_ids.back(), columns);

  auto table_block_bytes = serial::serialize_table_metadata(table_metadata, disk_manager->BLOCK_SIZE);

  disk_manager->write_block(table_block_bytes, table_metadata.table_block_id);

  sector1_bytes = serial::serialize_sector1(sector1_meta);
  disk_manager->write_sector(sector1_bytes, 1);

  return 1;
}

bool Megatron::search_table(std::string table_name, serial::TableMetadata &table_metadata) {
  serial::TableMetadata table;

  // Se carga todo sector 1
  std::vector<unsigned char> sector1_bytes;
  disk_manager->read_sector(sector1_bytes, 1);

  auto sector1_meta = serial::deserialize_sector1(sector1_bytes);

  // Se lee todos los bloques con tablas
  for (size_t i{}; i < sector1_meta.n_tables; ++i) {
    std::vector<unsigned char> block_bytes;

    // TODO: Cambio logica a buffer manager
    disk_manager->read_block(block_bytes, sector1_meta.table_block_ids[i]);

    serial::TableMetadata curr_table_metadata =
        serial::deserialize_table_metadata(block_bytes);

    if (array_to_string_view(curr_table_metadata.name) == table_name) {
      table_metadata = curr_table_metadata;
      return true;
    }
  }

  return false;
}

float Megatron::table_size(std::string table_name) {
  serial::TableMetadata table_metadata;
  ;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return 0;
  }

  // Revisamos todos los bloques asociados a file
  // auto blocks = extract_blocks_from_schema(table_name);

  // TODO: En base a lbas, revisa header para bytes libres/usados
  size_t total_size{};
  // for (size_t page_index{1}, page_lba{};
  //      page_index < file_header.n_blocks; ++page_index) {
  //   page_lba = table_metadata.file_lba +
  //              static_cast<size_t>(std::ceil((page_index * file_header.block_size) /
  //                                            static_cast<double>(disk_manager::SECTOR_SIZE)));
  //   std::vector<unsigned char> bytes;
  //   disk.read_block(bytes, file_header.block_size, page_lba);
  //
  //   auto fixed_block_header = serial::deserialize_fixed_block_header(bytes);
  //
  //   auto it_page_byte = bytes.begin();
  //
  //   // Revisamos todo registro marcado como ocupado
  //   for (size_t bmp_i{}; bmp_i < fixed_block_header.max_n_regs; ++bmp_i) {
  //     // Se encuentra registro
  //     if (fixed_block_header.free_register_bitmap.at(bmp_i)) {
  //       total_size += fixed_block_header.reg_size;
  //     }
  //   }
  // }

  // std::cout << metadata.tuple_size << " " << metadata.n_rows << std::endl;
  // return 1.f * metadata.tuple_size * metadata.n_rows / 1024;
  return total_size / 1024.;
}

// void Megatron::update(std::string cond_col_name, std::string condition, std::string repl_col_name, std::string new_value) {
// TableMetadata metadata = read_table(disk.FIRST_DBMS_LBA);
// size_t idx_condition_col{}, idx_replaced_col{};
// size_t offset_cond{}, offset_replaced{};
//
// // Revisar si condicion existe y offset para esta
// if (!cond_col_name.empty()) {
//   bool found{false};
//   for (auto &p : metadata.col_type_pairs) {
//     if (p.first == cond_col_name) {
//       found = true;
//       break;
//     }
//
//     if (p.second == "STR") {
//       offset_cond += 128;
//     } else {
//       offset_cond += 4;
//     }
//
//     ++idx_condition_col;
//   }
//   if (!found || condition.empty()) {
//     std::cout << "Condicional invalida" << std::endl;
//     return;
//   }
// }
// // std::cout << "Columna condicion encontrada: " << metadata.col_type_pairs[idx_condition_col].first << std::endl;
//
// // Revisar si condicion existe y offset para esta
// if (!repl_col_name.empty()) {
//   bool found{false};
//   for (auto &p : metadata.col_type_pairs) {
//     if (p.first == repl_col_name) {
//       found = true;
//       break;
//     }
//
//     if (p.second == "STR") {
//       offset_replaced += 128;
//     } else {
//       offset_replaced += 4;
//     }
//
//     ++idx_replaced_col;
//   }
//   if (!found || new_value.empty()) {
//     std::cout << "Condicional invalida" << std::endl;
//     return;
//   }
// }
// // std::cout << "Columna replace encontrada: " << metadata.col_type_pairs[idx_replaced_col].first << std::endl;
//
// for (size_t idx_row{}; idx_row < metadata.n_rows; ++idx_row) {
//   // if (idx_row == 27)
//   //   std::cout << std::endl;
//   std::vector<unsigned char> bytes;
//   disk.read_block(bytes, 1024, metadata.lbas[idx_row]);
//   auto ptr = bytes.data();
//
//   // Check de condicion
//   if (!condition.empty()) {
//     std::string cond_string;
//     if (metadata.col_type_pairs[idx_condition_col].second == "STR") {
//       cond_string.resize(20);
//       condition.resize(20);
//
//       std::memcpy(cond_string.data(), ptr + offset_cond, 20);
//     } else {
//       cond_string.resize(4);
//       condition.resize(4);
//
//       std::memcpy(cond_string.data(), ptr + offset_cond, 4);
//     }
//
//     if (cond_string != condition)
//       continue;
//
//     if (metadata.col_type_pairs[idx_replaced_col].second == "STR") {
//       new_value.resize(128);
//
//       std::memcpy(ptr + offset_replaced, new_value.data(), 128);
//
//     } else {
//       new_value.resize(4);
//
//       std::memcpy(ptr + offset_replaced, new_value.data(), 4);
//     }
//
//     // Se escribe con offset
//     disk.write_block(bytes, metadata.lbas[idx_row]);
//   }
// }
// }
