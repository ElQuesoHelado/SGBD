#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "types.hpp"
#include <cstddef>

/*
 * Retorna los bytes de un registro arbitrario en una pagina fija
 */
std::vector<unsigned char> Megatron::get_ith_register_bytes(
    const serial::TableMetadata &table_metadata,
    const serial::PageHeader &page_header,
    const serial::FixedDataHeader &fixed_data_header,
    const std::vector<unsigned char> &page_bytes,
    size_t ith_reg) {

  // Calculo de byte inicial del registro
  size_t reg_offset = serial::calculate_reg_offset(fixed_data_header, ith_reg);

  // Se extraen bytes de la pagina en ese offset
  std::vector<unsigned char> register_bytes(
      page_bytes.begin() + reg_offset,
      page_bytes.begin() + reg_offset + table_metadata.max_reg_size);

  return register_bytes;
}

// Asume que el slot esta usado
std::vector<unsigned char> Megatron::get_ith_register_bytes(
    const serial::TableMetadata &table_metadata,
    const serial::PageHeader &page_header,
    const serial::SlottedDataHeader &slotted_data_header,
    const std::vector<unsigned char> &page_bytes,
    size_t ith_reg) {

  // Calculo de byte inicial del registro
  size_t reg_offset = slotted_data_header.slots[ith_reg].offset_reg_start,
         reg_size = slotted_data_header.slots[ith_reg].reg_size;

  // Se extraen bytes de la pagina en ese offset
  std::vector<unsigned char> register_bytes(
      page_bytes.begin() + reg_offset,
      page_bytes.begin() + reg_offset + reg_size);

  return register_bytes;
}

std::vector<unsigned char> Megatron::serialize_register(
    const serial::TableMetadata &table_metadata,
    std::vector<std::string> &values) {
  if (values.size() != table_metadata.n_cols)
    throw std::runtime_error("Columnas a insertar diferentes a tabla");

  std::vector<unsigned char> register_bytes;
  register_bytes.reserve(table_metadata.max_reg_size);

  for (size_t col{}; col < table_metadata.n_cols; ++col) {
    auto temp_val = string_to_sql_type(values[col],
                                       table_metadata.columns[col].type,
                                       table_metadata.columns[col].max_size);

    auto sql_type_bytes = serialize_sql_type(temp_val);

    register_bytes.insert(register_bytes.end(),
                          sql_type_bytes.begin(),
                          sql_type_bytes.end());
  }

  return register_bytes;
}

std::vector<SQL_type> Megatron::deserialize_register(
    const serial::TableMetadata &table_metadata,
    std::vector<unsigned char> &register_bytes) {

  if (register_bytes.size() > table_metadata.max_reg_size)
    throw std::runtime_error("Bytes de registro mas grande que tamanio maximo de reg");

  // std::vector<unsigned char> register_bytes;
  // register_bytes.reserve(table_metadata.max_reg_size);
  std::vector<SQL_type> register_values;

  auto register_bytes_it = register_bytes.begin();
  for (size_t col{}; col < table_metadata.n_cols; ++col) {
    auto temp_val = deserialize_sql_type(register_bytes_it,
                                         table_metadata.columns[col].type,
                                         table_metadata.columns[col].max_size);

    register_values.push_back(temp_val);
  }

  return register_values;
}
