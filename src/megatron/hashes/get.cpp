#include "megatron.hpp"
#include "serial/page_header.hpp"
#include "types.hpp"
#include <cmath>
#include <cstddef>
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

size_t Megatron::get_depth(serial::TableMetadata &table_metadata, size_t col_index) {
  auto page_id = get_root_page_id(table_metadata, col_index);
  size_t counter{};

  while (page_id != disk_manager->NULL_BLOCK) {
    auto &frame = buffer_manager->load_pin_page(page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;
    auto it = page_bytes.begin();

    auto page_header = serial::deserialize_page_header(it);
    counter += page_header.n_regs;

    page_id = page_header.next_block_id;

    buffer_manager->free_unpin_page(page_id, false);
  }

  return std::log2(counter);
}
