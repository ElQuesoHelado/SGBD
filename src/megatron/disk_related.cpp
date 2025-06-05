// #include "disk_manager.hpp"
#include "disk_manager.hpp"
#include "megatron.hpp"
#include "serial/generic.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>

/* Creacion de una tabla en disco(metadata), tamanio de todo dato es arbitrario
 * TODO: POR AHORA SE RESERVA 1 archivo por tabla
 * Limpiar creacion
 */
// size_t Megatron::write_table_metadata(serial::TableMetadata &metadata) {
//   auto bytes = serialize_table_metadata(metadata);
//
//   auto lba = disk.write_block_dbms(bytes);
//
//   important_bytes << "Creacion tabla en: " << lba << " " << disk.logic_sector_to_byte(lba) << std::endl;
//
//   important_bytes << "Primer archivo en: " << metadata.file_lba << " " << disk.logic_sector_to_byte(metadata.file_lba) << std::endl;
//
//   return lba;
// }

// serial::Block Megatron::extract_block_from_schema(const std::string &table_name, size_t n_block) {
//   std::vector<serial::Block> blocks;
//   const std::string file_path = schema_dir_path + table_name + ".txt";
//
//   std::ifstream file(file_path);
//   if (!file.is_open()) {
//     throw std::runtime_error("No se pudo abrir el archivo de tabla: " + table_name + ".txt");
//   }
//
//   std::string line;
//   serial::Block block;
//
//   size_t current_line{};
//   while (std::getline(file, line)) {
//     if (current_line == n_block) {
//       std::istringstream iss(line);
//       size_t lba;
//
//       while (iss >> lba) {
//         block.lbas.push_back(lba);
//       }
//
//       // Espacio en blanco
//       if (block.lbas.empty()) {
//         return {};
//       }
//
//       return block;
//     }
//     current_line++;
//   }
//
//   // throw std::runtime_error("El bloque " + std::to_string(n_block) + " no existe en el archivo");
//   return {};
// }
//
// void Megatron::append_block_to_table(const std::string &table_name, const serial::Block &block) {
//   std::ofstream table_blocks_file(schema_dir_path + table_name + ".txt", std::ios::app),
//       blocks_file(schema_dir_path + table_name + "_blocks.txt", std::ios::app);
//
//   if (!table_blocks_file.is_open()) {
//     throw std::runtime_error("No se pudo abrir file de bloques de " + table_name + ".txt");
//   }
//
//   // TODO: check con bloques de tabla
//   if (block.lbas.empty()) {
//     throw std::runtime_error("Bloque no valido");
//   }
//
//   for (size_t i = 0; i < block.lbas.size(); ++i) {
//     if (i != 0) {
//       table_blocks_file << " ";
//     }
//     table_blocks_file << block.lbas[i];
//     blocks_file << disk.logic_sector_to_CHS(block.lbas[i]) << "    ";
//   }
//
//   table_blocks_file << "\n";
//   blocks_file << std::endl;
//
//   if (table_blocks_file.bad()) {
//     throw std::runtime_error("Error al escribir en el archivo de tabla");
//   }
// }

// Retorna -1 caso no exista un nth registro insertable
// int Megatron::is_fixed_block_insertable(const serial::Block &block) {
//   std::vector<unsigned char> bytes;
//   for (size_t i{}; i < n_sectors_in_block; ++i) {
//     disk.read_sector(bytes, disk.SECTOR_SIZE, block.lbas[i]);
//     auto header = serial::deserialize_fixed_block_header(bytes);
//     if (header.free_bytes >= header.reg_size)
//       return i;
//   }
//
//   return -1;
// }
//
// /* Calcula si se tiene capacidad suficiente,
//  * @note Toma en cuenta el agregado de size de Slot
//  */
// int Megatron::is_slotted_block_insertable(const serial::Block &block, size_t reg_size) {
//   std::vector<unsigned char> bytes;
//   for (size_t i{}; i < n_sectors_in_block; ++i) {
//     bytes.clear();
//     disk.read_sector(bytes, disk.SECTOR_SIZE, block.lbas[i]);
//     auto header = serial::deserialize_slotted_data_header(bytes);
//     if (header.free_bytes >= reg_size + sizeof(serial::Slot))
//       return i;
//   }
//
//   return -1;
// }

// serial::FileHeader get_free_file(serial::TableMetadata &table_metadata) {
// }

// std::vector<unsigned char> Megatron::get_n_page_from_file(size_t file_header_lba, serial::FileHeader &file_header, size_t n) {
//   std::vector<unsigned char> page_bytes;
//   disk.read_block(page_bytes, file_header.block_size,
//                   file_header_lba + (n * file_header.block_size) / disk_manager::SECTOR_SIZE);
//
//   return serial::deserialize_fixed_block_header(page_bytes);
// }

// size_t Megatron::write_empty_file(serial::FileHeader &file_header, serial::FixedDataHeader &fixed_block_header) {
//   // std::vector<unsigned char> whole_file(file_header.n_blocks * file_header.block_size);
//   std::vector<unsigned char> whole_file;
//   // const auto *ptr = whole_file.data();
//
//   // Primer bloque contiene file_header
//   std::vector<unsigned char> block;
//
//   auto file_header_bytes = serial::serialize_file(file_header);
//   file_header_bytes.resize(file_header.block_size);
//
//   whole_file.insert(whole_file.end(), file_header_bytes.begin(), file_header_bytes.end());
//
//   // Bloques genericos
//   for (size_t i{1}; i < file_header.n_blocks; ++i) {
//     auto fixed_block_header_bytes = serial::serialize_fixed_block_header(fixed_block_header);
//     fixed_block_header_bytes.resize(file_header.block_size);
//
//     whole_file.insert(whole_file.end(), fixed_block_header_bytes.begin(), fixed_block_header_bytes.end());
//   }
//
//   return disk.write_block(whole_file);
// }
//
// size_t Megatron::write_file_page(serial::TableMetadata &table_metadata, serial::FileHeader &file_header,
//                                  serial::FixedDataHeader &fixed_block_header, std::vector<unsigned char> &bytes, size_t index) {
//
//   auto written = serial::serialize_fixed_block_header(fixed_block_header);
//   written.insert(written.end(), bytes.begin(), bytes.end());
//
//   // disk.write_block(written, table_metadata.files_lbas[0] + file_header.block_size * index);
// }
