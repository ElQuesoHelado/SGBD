#include "megatron.hpp"
#include <print>
#include <string>

void Megatron::add_hash_to_table(std::string &table_name, std::string &col_name) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

  return add_hash_to_table(table_metadata, col_name);
}
void Megatron::add_hash_to_table(serial::TableMetadata &table_metadata, std::string &col_name) {
  auto col_index = get_column_index(table_metadata, col_name);

  // No existe columna
  if (col_index == table_metadata.n_cols) {
    std::println("Columna no existente para agregar hash");
    return;
  }

  return add_hash_to_table(table_metadata, col_index);
}

void Megatron::add_hash_to_table(serial::TableMetadata &table_metadata, size_t col_index) {
  // Busca si hash ya existe
  if (is_column_hashed(table_metadata, col_index)) {
    std::println("Columna ya tiene un hash asignado");
    return;
  }

  // Reserva de bloque para directorio principal
  auto dir_block_id = disk_manager->reserve_free_block();

  std::vector<std::string> values = {
      std::to_string(table_metadata.table_block_id),
      std::to_string(col_index),
      "0",
      std::to_string(dir_block_id)};

  // Se persiste
  insert(table_metadata, values);
}
