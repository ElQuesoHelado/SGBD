#include "megatron.hpp"
#include "result_set.hpp"
#include "serial/slotted_data.hpp"
#include <cstddef>

ResultSet Megatron::update_condition(std::string &table_name, std::string &col_name, std::string &condition) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }
  return update_condition(table_metadata, col_name, condition);
}
ResultSet update_condition(serial::TableMetadata &table_metadata, std::string &col_name, std::string &condition);
ResultSet update_from_page(serial::TableMetadata &table_metadata, size_t update_page_id, size_t col_index, SQL_type &cond_val);
ResultSet update_from_fixed_page(serial::TableMetadata &table_metadata, size_t update_page_id, size_t col_index, SQL_type &cond_val);
ResultSet update_from_slotted_page(serial::TableMetadata &table_metadata, size_t update_page_id, size_t col_index, SQL_type &cond_val);
