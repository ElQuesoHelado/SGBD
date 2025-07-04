#include "megatron.hpp"
#include "result_set.hpp"
#include "serial/fixed_data.hpp"
#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <cstddef>
#include <iostream>
#include <type_traits>

#include <print>

bool sql_in_range(const SQL_type &val, const SQL_type &low, const SQL_type &high) {
  // 1. Verificar que los tres sean del mismo tipo
  if (val.index() != low.index() || val.index() != high.index()) {
    return false;
  }

  // 2. Comparación por índice del variant
  switch (val.index()) {
  // Casos numéricos (índices 0-5)
  case 0:
    return std::get<0>(val) >= std::get<0>(low) && std::get<0>(val) <= std::get<0>(high); // int8_t
  case 1:
    return std::get<1>(val) >= std::get<1>(low) && std::get<1>(val) <= std::get<1>(high); // int16_t
  case 2:
    return std::get<2>(val) >= std::get<2>(low) && std::get<2>(val) <= std::get<2>(high); // int32_t
  case 3:
    return std::get<3>(val) >= std::get<3>(low) && std::get<3>(val) <= std::get<3>(high); // int64_t
  case 4:
    return std::get<4>(val) >= std::get<4>(low) && std::get<4>(val) <= std::get<4>(high); // float
  case 5:
    return std::get<5>(val) >= std::get<5>(low) && std::get<5>(val) <= std::get<5>(high); // double

  // Casos no numéricos (índices 6-7)
  default:
    return false; // CharType (6) y VarcharType (7)
  }
}

// TODO: Nombre columnas
void Megatron::select_print_range(std::string &table_name, std::string &col_name, std::string &low, std::string &high, int max_pages_loaded) {
  auto result_set = select_range(table_name, col_name, low, high, max_pages_loaded);

  size_t i{1};
  for (auto &reg : result_set) {
    std::println("{} {}", i, reg);
    i++;
  }
}

void Megatron::select_print_range(serial::TableMetadata &table_metadata, std::string &col_name, std::string &low, std::string &high, int max_pages_loaded) {
  auto result_set = select_range(table_metadata, col_name, low, high, max_pages_loaded);

  size_t i{1};
  for (auto &reg : result_set) {
    std::println("{} {}", i, reg);
    i++;
  }
}

ResultSet Megatron::select_range(std::string &table_name, std::string &col_name, std::string &low, std::string &high, int max_pages_loaded) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }

  return select_range(table_metadata, col_name, low, high, max_pages_loaded);
}

/*
 * Realiza el cargado de datos del disco
 * Considera campos solo para output y condiciones
 * ex: se quiere dos campos pero la condicion depende de otro no visualizado
 * @note caso no coincida col_name, se realiza un select sin condicion
 */
ResultSet Megatron::select_range(serial::TableMetadata &table_metadata, std::string &col_name, std::string &low, std::string &high, int max_pages_loaded) {
  // Se parsea column index y condicion a SQL_type
  size_t col_index{table_metadata.n_cols};
  SQL_type low_val, high_val;

  for (size_t i{}; i < table_metadata.columns.size(); ++i) {
    if (col_name == array_to_string_view(table_metadata.columns[i].name)) {
      col_index = i;
      low_val = string_to_sql_type(low, table_metadata.columns[i].type, table_metadata.columns[i].max_size);
      high_val = string_to_sql_type(high, table_metadata.columns[i].type, table_metadata.columns[i].max_size);
    }
  }

  ResultSet result_set{};
  result_set.add_columns(table_metadata.columns);

  size_t curr_page_id = table_metadata.first_page_id;
  while (curr_page_id != disk_manager->NULL_BLOCK && max_pages_loaded != 0) {
    auto frame = buffer_manager->load_pin_page(curr_page_id);
    max_pages_loaded--;

    std::vector<unsigned char> &page_bytes = frame.page_bytes;
    auto page_header = serial::deserialize_page_header(page_bytes);

    buffer_manager->free_unpin_page(curr_page_id, false);

    auto partial_result_set = select_from_page_range(table_metadata, curr_page_id, col_index, low_val, high_val);

    result_set.merge(std::move(partial_result_set));

    curr_page_id = page_header.next_block_id;
  }

  return result_set;
}

ResultSet Megatron::select_from_page_range(serial::TableMetadata &table_metadata, size_t select_page_id, size_t col_index, SQL_type &low_val, SQL_type &high_val) {
  auto result_set =
      (table_metadata.are_regs_fixed)
          ? select_from_fixed_page_range(table_metadata, select_page_id, col_index, low_val, high_val)
          : select_from_slotted_page_range(table_metadata, select_page_id, col_index, low_val, high_val);

  return result_set;
}

ResultSet Megatron::select_from_fixed_page_range(serial::TableMetadata &table_metadata, size_t select_page_id, size_t col_index, SQL_type &low_val, SQL_type &high_val) {
  auto frame = buffer_manager->load_pin_page(select_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;

  auto page_bytes_it = page_bytes.begin();

  // Se saca metadata relevante
  auto page_header = serial::deserialize_page_header(page_bytes_it);
  auto fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);

  ResultSet result_set;
  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
      auto register_bytes = get_ith_register_bytes(table_metadata, page_header, fixed_data_header, page_bytes, i);
      auto register_values = deserialize_register(table_metadata, register_bytes);

      // Si es que hay condicion
      if (col_index < table_metadata.n_cols && sql_in_range(register_values[col_index], low_val, high_val))
        continue;

      RegisterEntry reg{select_page_id, i};

      for (auto &v : register_values)
        reg.values.push_back(SQL_type_to_string(v));
      // std::cout << SQL_type_to_string(v) << " | ";

      // std::cout << std::endl;
      result_set.add_register(std::move(reg));
    }
  }

  buffer_manager->free_unpin_page(select_page_id, 0);
  return result_set;
}

ResultSet Megatron::select_from_slotted_page_range(serial::TableMetadata &table_metadata, size_t select_page_id, size_t col_index, SQL_type &low_val, SQL_type &high_val) {
  auto frame = buffer_manager->load_pin_page(select_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;

  auto page_bytes_it = page_bytes.begin();

  // Se saca metadata relevante
  auto page_header = serial::deserialize_page_header(page_bytes_it);
  auto slotted_data_header = serial::deserialize_slotted_data_header(page_bytes_it);

  ResultSet result_set;
  for (size_t i{}; i < slotted_data_header.n_slots; ++i) {
    if (slotted_data_header.slots[i].is_used) { // Registro existe
      auto register_bytes = get_ith_register_bytes(table_metadata, page_header,
                                                   slotted_data_header, page_bytes, i);
      auto register_values = deserialize_register(table_metadata, register_bytes);

      // Si es que hay condicion
      if (col_index < table_metadata.n_cols && sql_in_range(register_values[col_index], low_val, high_val))
        continue;

      RegisterEntry reg{select_page_id, i};

      for (auto &v : register_values)
        reg.values.push_back(SQL_type_to_string(v));
      // std::cout << SQL_type_to_string(v) << " | ";

      // std::cout << std::endl;

      result_set.add_register(std::move(reg));
    }
  }

  buffer_manager->free_unpin_page(select_page_id, 0);

  return result_set;
}
