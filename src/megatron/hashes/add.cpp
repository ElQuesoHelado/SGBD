#include "hlpr.hpp"
#include "megatron.hpp"
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>

size_t Megatron::create_dir_page() {
  return create_fixed_page(sizeof(uint32_t));
}

// TODO: check sizes, ?tal vez pos no necesita un size_t?
size_t Megatron::create_bucket_page() {
  return create_fixed_page(2 * sizeof(uint32_t));
}

void Megatron::add_hash_to_table(std::string &table_name, std::string &col_name, size_t initial_depth) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

  return add_hash_to_table(table_metadata, col_name, initial_depth);
}
void Megatron::add_hash_to_table(serial::TableMetadata &table_metadata, std::string &col_name, size_t initial_depth) {
  auto col_index = get_column_index(table_metadata, col_name);

  // No existe columna
  if (col_index == table_metadata.n_cols) {
    std::println("Columna no existente para agregar hash");
    return;
  }

  return add_hash_to_table(table_metadata, col_index, initial_depth);
}

void Megatron::add_hash_to_table(serial::TableMetadata &table_metadata, size_t col_index, size_t initial_depth) {
  // Busca si hash ya existe
  if (is_column_hashed(table_metadata, col_index)) {
    std::println("Columna ya tiene un hash asignado");
    return;
  }

  // Creamos una pagina inicial para directorio
  auto new_dir_id = create_dir_page();

  std::vector<std::string> values = {
      std::to_string(table_metadata.table_block_id),
      std::to_string(col_index),
      "0",
      std::to_string(new_dir_id)};

  // Se persiste
  insert(catalog_name, values);

  // Tener un depth mayor a 0 es mas eficiente(cc se tiene una busqueda lineal hasta llenarlo)
  // Se asume depth = 1, por propiedad 2^1 = 2
  // 2 buckets iniciales
  for (size_t i{}; i < std::pow(2, initial_depth); ++i) {
    auto new_bucket_id = create_dir_page();
    auto pointer_bytes = serialize_pointer(new_bucket_id);
    insert_into_fixed_page(new_dir_id, pointer_bytes);
  }
}
