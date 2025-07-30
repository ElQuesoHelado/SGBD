#include "megatron.hpp"
#include "serial/fixed_page.hpp"
#include "serial/slotted_page.hpp"
#include <iostream>

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
    insert_page_id =
        get_insertable_page_id(table_metadata.first_page_id,
                               table_metadata.max_reg_size);
  } else {
    // Peor caso, siempre se crea nuevo Slot
    insert_page_id =
        get_insertable_page_id(
            table_metadata.first_page_id,
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

  // Hashes
  auto hashed_cols = get_hashed_columns(table_metadata);
  for (auto [col, page_id] : hashed_cols) {
    Hasher hasher(*buffer_manager,
                  page_id,
                  table_metadata.columns[col].type,
                  table_metadata.columns[col].max_size);
    hasher.insert_from_set(result_set, col);
  }

  // Indices
  auto indexed_cols = get_indexed_columns(table_metadata);
  for (auto [col, page_id] : indexed_cols) {
    size_t min_degree =
        calculate_btree_order(table_metadata.columns[col].max_size);
    BPTree tree(*buffer_manager, table_metadata,
                page_id,
                min_degree,
                table_metadata.columns[col].type,
                table_metadata.columns[col].max_size);

    for (auto &reg : result_set) {
      tree.insert(
          reg.values[col],
          {static_cast<uint32_t>(reg.page_id),
           static_cast<uint16_t>(reg.position)});
    }
  }

  return result_set;
}

size_t Megatron::insert_into_fixed_page(uint32_t insert_page_id,
                                        std::vector<unsigned char> &register_bytes) {
  auto &frame = buffer_manager->load_pin_page(insert_page_id);

  std::span<unsigned char> insert_page_data(frame.page_bytes);

  serial::FixedPage fixed_page(insert_page_data, frame.page_bytes);

  auto free_reg_pos = fixed_page.insert_register_bytes(register_bytes);

  buffer_manager->free_unpin_page(insert_page_id, true);

  return free_reg_pos;
}

size_t Megatron::insert_into_slotted_page(
    uint32_t insert_page_id,
    std::vector<unsigned char> &register_bytes) {
  // Se lee pagina y saca metadata relevante
  auto &frame = buffer_manager->load_pin_page(insert_page_id);

  std::span<unsigned char> insert_page_data(frame.page_bytes);

  serial::SlottedPage slotted_page(insert_page_data, frame.page_bytes);

  auto free_slot = slotted_page.insert_register_bytes(register_bytes);

  buffer_manager->free_unpin_page(insert_page_id, true);

  return free_slot;
}
