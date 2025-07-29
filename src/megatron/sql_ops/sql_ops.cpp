#include "megatron.hpp"
#include "serial/sector1.hpp"
#include <iostream>

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
  // sector1_meta.table_block_ids.push_back(disk_manager->reserve_free_block());
  auto new_table_frame = buffer_manager->get_load_free_frame();
  sector1_meta.table_block_ids.push_back(new_table_frame.page_id);

  init_table_metadata(table_metadata, name, new_table_frame.page_id, columns);

  auto table_block_bytes =
      serial::serialize_table_metadata(
          table_metadata, disk_manager->BLOCK_SIZE);

  // disk_manager->write_block(table_block_bytes, table_metadata.table_block_id);
  buffer_manager->free_unpin_page(new_table_frame.page_id, true);

  sector1_bytes = serial::serialize_sector1(sector1_meta);
  disk_manager->write_sector(sector1_bytes, 1);

  return 1;
}

bool Megatron::search_table(std::string table_name,
                            serial::TableMetadata &table_metadata) {
  serial::TableMetadata table;

  // Se carga todo sector 1
  std::vector<unsigned char> sector1_bytes;
  disk_manager->read_sector(sector1_bytes, 1);

  auto sector1_meta = serial::deserialize_sector1(sector1_bytes);

  // Se lee todos los bloques con tablas
  for (size_t i{}; i < sector1_meta.n_tables; ++i) {

    // disk_manager->read_block(block_bytes, sector1_meta.table_block_ids[i]);

    auto table_id = sector1_meta.table_block_ids[i];

    auto frame = buffer_manager->load_pin_page(table_id);
    std::vector<unsigned char> &block_bytes = frame.page_bytes;

    serial::TableMetadata curr_table_metadata =
        serial::deserialize_table_metadata(block_bytes);

    if (array_to_string_view(curr_table_metadata.name) == table_name) {
      table_metadata = curr_table_metadata;
      buffer_manager->free_unpin_page(table_id, 0);
      return true;
    }

    buffer_manager->free_unpin_page(table_id, 0);
  }

  return false;
}

bool Megatron::search_table(size_t table_id,
                            serial::TableMetadata &table_metadata) {
  serial::TableMetadata table;

  // Se carga todo sector 1
  std::vector<unsigned char> sector1_bytes;
  disk_manager->read_sector(sector1_bytes, 1);

  auto sector1_meta = serial::deserialize_sector1(sector1_bytes);

  // Se lee todos los bloques con tablas
  for (size_t i{}; i < sector1_meta.n_tables; ++i) {

    // disk_manager->read_block(block_bytes, sector1_meta.table_block_ids[i]);

    auto curr_table_id = sector1_meta.table_block_ids[i];

    auto frame = buffer_manager->load_pin_page(curr_table_id);
    std::vector<unsigned char> &block_bytes = frame.page_bytes;

    serial::TableMetadata curr_table_metadata =
        serial::deserialize_table_metadata(block_bytes);

    if (table_id == curr_table_id) {
      table_metadata = curr_table_metadata;
      buffer_manager->free_unpin_page(curr_table_id, 0);
      return true;
    }

    buffer_manager->free_unpin_page(curr_table_id, 0);
  }

  return false;
}
