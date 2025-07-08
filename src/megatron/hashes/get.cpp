#include "megatron.hpp"
#include "types.hpp"
#include <cstdint>
#include <print>
#include <string>

bool Megatron::is_column_hashed(std::string &table_name, std::string &col_name) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return false;
  }

  return is_column_hashed(table_metadata, col_name);
}

bool Megatron::is_column_hashed(serial::TableMetadata &table_metadata, std::string &col_name) {
  auto col_index = get_column_index(table_metadata, col_name);

  // No existe columna
  if (col_index == table_metadata.n_cols) {
    std::println("Columna no existente en tabla");
    return false;
  }

  return is_column_hashed(table_metadata, col_index);
}

bool Megatron::is_column_hashed(serial::TableMetadata &table_metadata, size_t col_index) {
  // Se busca en catalog entrada: (table_id, col_index, hash_type:0)
  // FIXME: Check de tipos
  std::vector<std::tuple<size_t, std::string, SQL_type>>
      comparisons = {
          {0, "==", std::int32_t{(int)table_metadata.table_block_id}},
          {1, "==", std::int32_t{(int)col_index}},
          {2, "==", std::int32_t{0}},

      };

  auto comparator = generate_comparator(table_metadata, comparisons);

  auto results = select(table_metadata, comparator);
  return !results.empty();
}
