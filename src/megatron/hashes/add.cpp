#include "hash/hasher.hpp"
#include "megatron.hpp"
#include <cstddef>
#include <cstdint>
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

void Megatron::add_hash_to_table(serial::TableMetadata &table_metadata,
                                 size_t col_index) {
  // Busca si hash ya existe
  if (is_column_hashed(table_metadata, col_index)) {
    std::println("Columna ya tiene un hash asignado");
    return;
  }

  Hasher hasher(*buffer_manager,
                disk_manager->NULL_BLOCK,
                table_metadata.columns[col_index].type,
                table_metadata.columns[col_index].max_size);

  std::vector<std::string>
      values = {
          std::to_string(table_metadata.table_block_id),
          std::to_string(col_index),
          "0",
          std::to_string(hasher.directory_id)};

  insert(catalog_name, values);

  Comparator comp;
  auto registers = select(table_metadata, comp);

  for (auto &reg : registers) {
    hasher.insert(
        reg.values[col_index],
        {static_cast<uint32_t>(reg.page_id),
         static_cast<uint16_t>(reg.position)});
  }
}
