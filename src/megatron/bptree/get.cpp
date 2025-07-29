#include "megatron.hpp"
#include <iostream>
#include <print>

bool Megatron::is_column_indexed(
    std::string &table_name, std::string &col_name) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return false;
  }

  return is_column_indexed(table_metadata, col_name);
}

bool Megatron::is_column_indexed(serial::TableMetadata &table_metadata,
                                 std::string &col_name) {
  auto col_index = get_column_index(table_metadata, col_name);

  // No existe columna
  if (col_index == table_metadata.n_cols) {
    std::println("Columna no existente en tabla");
    return false;
  }

  return is_column_indexed(table_metadata, col_index);
}

bool Megatron::is_column_indexed(serial::TableMetadata &table_metadata,
                                 size_t col_index) {
  // Se busca en catalog entrada: (table_id, col_index, index_type:1)
  std::vector<std::tuple<size_t, std::string, SQL_type_>>
      comparisons = {
          {0, "==", std::int32_t{(int)table_metadata.table_block_id}},
          {1, "==", std::int32_t{(int)col_index}},
          {2, "==", std::int32_t{1}},
      };

  auto comparator = generate_comparator(table_metadata, comparisons);

  auto results = select(catalog_name, comparator);
  return !results.empty();
}

std::pair<uint32_t, uint32_t>
Megatron::get_indexed_column(serial::TableMetadata &table_metadata,
                             size_t col_index) {
  // Se busca en catalog entrada: (table_id, col_index, index_type:1)
  std::vector<std::tuple<size_t, std::string, SQL_type_>>
      comparisons = {
          {0, "==", std::int32_t{(int)table_metadata.table_block_id}},
          {1, "==", std::int32_t{(int)col_index}},
          {2, "==", std::int32_t{1}},
      };

  auto comparator = generate_comparator(table_metadata, comparisons);

  auto results = select(catalog_name, comparator);

  if (results.empty())
    return {};

  return {
      std::get<int>(results.registers.front().values[1]),
      std::get<int>(results.registers.front().values[3])};
}

std::vector<std::pair<uint32_t, uint32_t>>
Megatron::get_indexed_columns(std::string &table_name) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }

  return get_indexed_columns(table_metadata);
}

//(col_index, root_id)
std::vector<std::pair<uint32_t, uint32_t>>
Megatron::get_indexed_columns(serial::TableMetadata &table_metadata) {
  // Se busca en catalog entrada: (table_id, col_index, index_type:1)
  std::vector<std::tuple<size_t, std::string, SQL_type_>>
      comparisons = {
          {0, "==", std::int32_t{(int)table_metadata.table_block_id}},
          {2, "==", std::int32_t{1}},
      };

  auto comparator = generate_comparator(table_metadata, comparisons);

  auto result_set = select(catalog_name, comparator);

  std::vector<std::pair<uint32_t, uint32_t>> indexed_columns;
  for (auto &r : result_set) {
    indexed_columns.emplace_back(std::get<int>(r.values[1]),
                                 std::get<int>(r.values[3]));
  }

  return indexed_columns;
}
