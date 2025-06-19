#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

void Megatron::insert(std::string table_name, std::vector<std::string> &values) {
  serial::TableMetadata table_metadata;

  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla " + table_name + " no encontrada para insert" << std::endl;
    return;
  }

  if (table_metadata.are_regs_fixed)
    insert_fixed(table_metadata, values);
  else
    insert_slotted(table_metadata, values);
}

// Inserta un solo registro en tabla
void Megatron::insert_fixed(serial::TableMetadata &table_metadata, std::vector<std::string> &values) {
  std::cout << values[0] << std::endl;
  if (values[0] == "27")
    int x = 5;

  if (table_metadata.columns.size() != values.size()) {
    std::cerr << "Numero de valores diferente a columnas" << std::endl;
    return;
  }

  // Serializamos todo el registro
  auto register_bytes = serialize_register(table_metadata,
                                           values);

  if (register_bytes.size() > table_metadata.max_reg_size)
    throw std::runtime_error("Registro serializado mas grande que size maximo de registro");

  // Se busca pagina a insertar
  uint32_t insert_page_id;

  insert_page_id = get_insertable_page(table_metadata.first_page_id,
                                       table_metadata.max_reg_size);

  // Paginas sin espacio suficiente
  if (insert_page_id == disk.NULL_BLOCK)
    insert_page_id = add_new_page_to_table(table_metadata);

  // Se lee pagina y saca metadata relevante
  serial::PageHeader page_header;
  serial::FixedDataHeader fixed_data_header;

  auto &frame = buffer_manager_ptr->get_block(insert_page_id);
  std::vector<unsigned char> &insert_page_bytes = frame.page_bytes;

  // disk.read_block(insert_page_bytes, insert_page_id);

  size_t byte_offset_free_reg;

  page_header = serial::deserialize_page_header(insert_page_bytes);

  fixed_data_header = serial::deserialize_fixed_data_header(insert_page_bytes);

  // Calculamos posicion donde insertar
  size_t free_reg_pos = serial::find_free_reg_pos(fixed_data_header);
  byte_offset_free_reg = serial::calculate_reg_offset(fixed_data_header,
                                                      free_reg_pos);

  if (free_reg_pos >= fixed_data_header.max_n_regs) {
    throw std::runtime_error("No hay registros libres en bitmap pero se intentó insertar");
  }

  // El write si procede
  frame.dirty = true;

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
  std::copy(register_bytes.begin(), register_bytes.end(), page_it);

  // disk.write_block(insert_page_bytes, insert_page_id);
}

void Megatron::insert_slotted(serial::TableMetadata &table_metadata, std::vector<std::string> &values) {

  // FIXME: caso ultima col vacia
  if (table_metadata.columns.size() != values.size()) {
    std::cerr << "Numero de valores diferente a columnas" << std::endl;
    return;
  }

  // Serializamos todo el registro
  auto register_bytes = serialize_register(table_metadata, values);

  if (register_bytes.size() > table_metadata.max_reg_size)
    throw std::runtime_error("Registro serializado mas grande que size maximo de registro");

  // Se busca pagina a insertar
  uint32_t insert_page_id;

  // Peor caso, siempre se crea nuevo Slot
  insert_page_id = get_insertable_page(table_metadata.first_page_id,
                                       register_bytes.size() + sizeof(serial::Slot));

  // Paginas sin espacio suficiente
  if (insert_page_id == disk.NULL_BLOCK)
    insert_page_id = add_new_page_to_table(table_metadata);

  // Se lee pagina y saca metadata relevante
  serial::PageHeader page_header;
  serial::SlottedDataHeader slotted_data_header;

  auto &frame = buffer_manager_ptr->get_block(insert_page_id);
  std::vector<unsigned char> &insert_page_bytes = frame.page_bytes;

  frame.dirty = true;

  // disk.read_block(insert_page_bytes, insert_page_id);

  size_t byte_offset_free_reg;

  page_header = serial::deserialize_page_header(insert_page_bytes);

  slotted_data_header = serial::deserialize_slotted_data_header(insert_page_bytes);

  size_t free_slot = serial::get_free_slot(slotted_data_header);
  if (free_slot == slotted_data_header.n_slots)
    free_slot = serial::add_free_slot(page_header, slotted_data_header);

  byte_offset_free_reg = serial::insert_into_slotted(slotted_data_header,
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

  page_it = insert_page_bytes.begin() + byte_offset_free_reg;

  // Copia registro como tal
  std::copy(register_bytes.begin(), register_bytes.end(), page_it);

  // disk.write_block(insert_page_bytes, insert_page_id);
}
