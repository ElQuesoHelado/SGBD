#include "megatron.hpp"
#include <iostream>
#include <print>

bool Megatron::is_column_hashed(std::string &table_name, std::string &col_name) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return false;
  }

  return is_column_hashed(table_metadata, col_name);
}

bool Megatron::is_column_hashed(serial::TableMetadata &table_metadata,
                                std::string &col_name) {
  auto col_index = get_column_index(table_metadata, col_name);

  // No existe columna
  if (col_index == table_metadata.n_cols) {
    std::println("Columna no existente en tabla");
    return false;
  }

  return is_column_hashed(table_metadata, col_index);
}

bool Megatron::is_column_hashed(serial::TableMetadata &table_metadata,
                                size_t col_index) {
  // Se busca en catalog entrada: (table_id, col_index, hash_type:0)
  // FIXME: Check de tipos
  std::vector<std::tuple<size_t, std::string, SQL_type_>>
      comparisons = {
          {0, "==", std::int32_t{(int)table_metadata.table_block_id}},
          {1, "==", std::int32_t{(int)col_index}},
          {2, "==", std::int32_t{0}},

      };

  auto comparator = generate_comparator(table_metadata, comparisons);

  auto results = select(catalog_name, comparator);
  return !results.empty();
}

std::vector<std::pair<uint32_t, uint32_t>>
Megatron::get_hashed_columns(std::string &table_name) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }

  return get_hashed_columns(table_metadata);
}

std::vector<std::pair<uint32_t, uint32_t>>
Megatron::get_hashed_columns(serial::TableMetadata &table_metadata) {
  // Se busca en catalog entrada: (table_id, col_index, hash_type:0)
  std::vector<std::tuple<size_t, std::string, SQL_type_>>
      comparisons = {
          {0, "==", std::int32_t{(int)table_metadata.table_block_id}},
          {2, "==", std::int32_t{0}},
      };

  auto comparator = generate_comparator(table_metadata, comparisons);

  auto result_set = select(catalog_name, comparator);

  std::vector<std::pair<uint32_t, uint32_t>> hashed_columns;
  for (auto &r : result_set) {
    hashed_columns.emplace_back(std::get<int>(r.values[1]),
                                std::get<int>(r.values[3]));
  }

  return hashed_columns;
}

uint32_t Megatron::get_root_page_id(serial::TableMetadata &table_metadata,
                                    size_t col_index) {
  std::vector<std::tuple<size_t, std::string, SQL_type_>>
      comparisons = {
          {0, "==", std::int32_t{(int)table_metadata.table_block_id}},
          {1, "==", std::int32_t{(int)col_index}},
          //{2, "==", std::int32_t{0}},
      };

  auto comparator = generate_comparator(table_metadata, comparisons);

  auto result_set = select(catalog_name, comparator);

  if (result_set.empty())
    return disk_manager->NULL_BLOCK;

  return std::get<int>(result_set.registers.front().values[3]);
}
