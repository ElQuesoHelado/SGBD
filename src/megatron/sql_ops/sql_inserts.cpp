#include "hash/hasher.hpp"
#include "megatron.hpp"
#include "result_set.hpp"
#include "serial/fixed_data.hpp"
#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

ResultSet Megatron::insert(std::string table_name, std::vector<std::string> &values) {
  serial::TableMetadata table_metadata;

  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla " + table_name + " no encontrada para insert" << std::endl;
    return {};
  }

  return insert(table_metadata, values);
}

ResultSet Megatron::insert(serial::TableMetadata &table_metadata, std::vector<std::string> &values) {
  if (table_metadata.columns.size() != values.size()) {
    std::cerr << "Numero de valores diferente a columnas" << std::endl;
    return {};
  }

  // Serializamos todo el registro
  auto register_bytes = serialize_register(table_metadata, values);

  if (register_bytes.size() > table_metadata.max_reg_size)
    throw std::runtime_error("Registro serializado mas grande que size maximo de registro");

  // Se busca pagina a insertar
  uint32_t insert_page_id;

  if (table_metadata.are_regs_fixed) {
    insert_page_id = get_insertable_page_id(table_metadata.first_page_id,
                                            table_metadata.max_reg_size);
  } else {
    // Peor caso, siempre se crea nuevo Slot
    insert_page_id = get_insertable_page_id(table_metadata.first_page_id,
                                            register_bytes.size() + sizeof(serial::Slot));
  }

  // Paginas sin espacio suficiente
  if (insert_page_id == disk_manager->NULL_BLOCK)
    insert_page_id = add_new_page_to_table(table_metadata);

  return insert_into_page(table_metadata, insert_page_id, register_bytes);
}

ResultSet Megatron::insert_into_page(serial::TableMetadata &table_metadata,
                                     uint32_t insert_page_id,
                                     std::vector<std::string> &reg_values) {
  if (table_metadata.columns.size() != reg_values.size()) {
    std::cerr << "Numero de valores diferente a columnas" << std::endl;
    return {};
  }

  // Serializamos todo el registro
  auto register_bytes =
      serialize_register(table_metadata, reg_values);

  if (register_bytes.size() > table_metadata.max_reg_size)
    throw std::runtime_error(
        "Registro serializado mas grande que size maximo de registro");

  return insert_into_page(table_metadata, insert_page_id, register_bytes);
}

ResultSet Megatron::insert_into_page(serial::TableMetadata &table_metadata,
                                     uint32_t insert_page_id,
                                     std::vector<unsigned char> &register_bytes) {
  size_t pos = (table_metadata.are_regs_fixed)
                   ? insert_into_fixed_page(insert_page_id, register_bytes)
                   : insert_into_slotted_page(insert_page_id, register_bytes);

  auto register_values =
      deserialize_register(table_metadata, register_bytes);

  RegisterEntry reg{static_cast<uint32_t>(insert_page_id),
                    static_cast<uint16_t>(pos)};

  for (auto &v : register_values)
    reg.values.push_back(v);

  ResultSet result_set;
  result_set.add_columns(table_metadata.columns);
  result_set.add_register(std::move(reg));

  // Check hashes
  auto cols = get_hashed_columns(table_metadata);
  for (auto [col, page_id] : cols) {
    Hasher hasher(*buffer_manager,
                  page_id,
                  table_metadata.columns[col].type,
                  table_metadata.columns[col].max_size);
    hasher.insert_from_set(result_set, col);
    // hasher.insert(const SQL_type &key, const RegPtr &reg_ptr)
  }

  // TODO: indexes

  return result_set;
}

size_t Megatron::insert_into_fixed_page(uint32_t insert_page_id,
                                        std::vector<unsigned char> &register_bytes) {
  auto &frame = buffer_manager->load_pin_page(insert_page_id);
  std::vector<unsigned char> &insert_page_bytes = frame.page_bytes;

  size_t byte_offset_free_reg;

  auto page_header =
      serial::deserialize_page_header(insert_page_bytes);

  auto fixed_data_header =
      serial::deserialize_fixed_data_header(insert_page_bytes);

  // Calculamos posicion donde insertar
  size_t free_reg_pos = serial::find_free_reg_pos(fixed_data_header);
  byte_offset_free_reg = serial::calculate_reg_offset(fixed_data_header,
                                                      free_reg_pos);

  if (free_reg_pos >= fixed_data_header.max_n_regs) {
    throw std::runtime_error(
        "No hay registros libres en bitmap pero se intentó insertar");
  }

  // El write si procede
  fixed_data_header.free_bytes -= fixed_data_header.reg_size;
  fixed_data_header.free_register_bitmap[free_reg_pos] = true;

  page_header.free_space -= fixed_data_header.reg_size;
  page_header.n_regs++;

  // Reemplazamos headers modificados
  auto page_it = insert_page_bytes.begin();
  {
    serial::serialize_page_header(page_header, page_it);
    serial::serialize_fixed_block_header(fixed_data_header, page_it);
  }

  page_it = insert_page_bytes.begin() + byte_offset_free_reg;

  // Copia registro como tal
  std::copy(register_bytes.begin(),
            register_bytes.end(), page_it);

  buffer_manager->free_unpin_page(insert_page_id, true);

  return free_reg_pos;
}

size_t Megatron::insert_into_slotted_page(
    uint32_t insert_page_id,
    std::vector<unsigned char> &register_bytes) {
  // Se lee pagina y saca metadata relevante
  auto &frame = buffer_manager->load_pin_page(insert_page_id);
  std::vector<unsigned char> &insert_page_bytes = frame.page_bytes;

  auto page_header = serial::deserialize_page_header(insert_page_bytes);
  auto slotted_data_header = serial::deserialize_slotted_data_header(insert_page_bytes);

  // Se actualiza headers para aceptar un registro nuevo
  size_t free_slot = serial::get_free_slot(slotted_data_header);
  if (free_slot == slotted_data_header.n_slots)
    free_slot = serial::add_free_slot(page_header, slotted_data_header);

  size_t byte_offset_free_reg = serial::prepare_slotted_insert(slotted_data_header,
                                                               free_slot,
                                                               register_bytes.size());

  page_header.free_space -= register_bytes.size();
  page_header.n_regs++;

  // Reemplazamos headers modificados
  auto page_it = insert_page_bytes.begin();
  {
    serial::serialize_page_header(page_header, page_it);
    serial::serialize_slotted_data_header(slotted_data_header, page_it);
  }

  // Insercion de registro en offset correcto
  page_it = insert_page_bytes.begin() + byte_offset_free_reg;
  std::copy(register_bytes.begin(), register_bytes.end(), page_it);

  buffer_manager->free_unpin_page(insert_page_id, true);

  return free_slot;
}
