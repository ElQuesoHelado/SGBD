#include "megatron.hpp"
#include "result_set.hpp"
#include "serial/slotted_data.hpp"
#include <cstddef>
#include <print>

ResultSet Megatron::update_condition(std::string &table_name,
                                     Comparator &comparator,
                                     std::string &upd_col_name,
                                     std::string &upd_col_value) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }
  return update_condition(table_metadata, comparator,
                          upd_col_name, upd_col_value);
}

ResultSet Megatron::update_condition(serial::TableMetadata &table_metadata,
                                     Comparator &comparator,
                                     std::string &upd_col_name,
                                     std::string &upd_col_value) {
  size_t upd_col_index{table_metadata.n_cols};
  SQL_type_ upd_value;

  for (size_t i{}; i < table_metadata.columns.size(); ++i) {
    if (upd_col_name ==
        array_to_string_view(table_metadata.columns[i].name)) {
      upd_col_index = i;
      upd_value = string_to_sql_type(upd_col_value,
                                     table_metadata.columns[i].type,
                                     table_metadata.columns[i].max_size);
    }
  }

  if (comparator.empty() ||
      upd_col_index >= table_metadata.n_cols) {
    std::cerr << "No se encontro columna para comparar/modificar" << std::endl;
    return {};
  }

  ResultSet result_set{};
  result_set.add_columns(table_metadata.columns);

  size_t curr_page_id = table_metadata.first_page_id;
  while (curr_page_id != disk_manager->NULL_BLOCK) {
    auto frame = buffer_manager->load_pin_page(curr_page_id);

    std::vector<unsigned char> &page_bytes = frame.page_bytes;
    auto page_header =
        serial::deserialize_page_header(page_bytes);

    buffer_manager->free_unpin_page(curr_page_id, false);

    auto partial_result_set =
        update_from_page(table_metadata, curr_page_id,
                         comparator, upd_col_index, upd_value);

    result_set.merge(std::move(partial_result_set));

    curr_page_id = page_header.next_block_id;
  }

  return result_set;
}

ResultSet Megatron::update_from_page(serial::TableMetadata &table_metadata,
                                     uint32_t update_page_id,
                                     Comparator &comparator,
                                     std::string &upd_col_name,
                                     std::string &upd_col_value) {
  // Valida input
  size_t upd_col_index{table_metadata.n_cols};
  SQL_type_ upd_value;

  for (size_t i{}; i < table_metadata.columns.size(); ++i) {
    if (upd_col_name ==
        array_to_string_view(table_metadata.columns[i].name)) {
      upd_col_index = i;
      upd_value = string_to_sql_type(upd_col_value,
                                     table_metadata.columns[i].type,
                                     table_metadata.columns[i].max_size);
    }
  }

  if (comparator.empty() ||
      upd_col_index >= table_metadata.n_cols) {
    std::cerr << "No se encontro columna para comparar/modificar" << std::endl;
    return {};
  }

  return update_from_page(table_metadata, update_page_id,
                          comparator, upd_col_index, upd_value);
}

ResultSet Megatron::update_from_page(
    serial::TableMetadata &table_metadata,
    uint32_t update_page_id,
    Comparator &comparator,
    size_t upd_col_index, SQL_type_ &upd_value) {
  auto result_set =
      (table_metadata.are_regs_fixed)
          ? update_from_fixed_page(table_metadata, update_page_id,
                                   comparator,
                                   upd_col_index, upd_value)
          : update_from_slotted_page(table_metadata, update_page_id,
                                     comparator,
                                     upd_col_index, upd_value);

  auto hashed_cols = get_hashed_columns(table_metadata);
  for (auto &[c, r] : hashed_cols) {
    Hasher hasher(
        *buffer_manager,
        r,
        table_metadata.columns[c].type,
        table_metadata.columns[c].max_size);

    for (auto &reg : result_set.registers) {
      hasher.remove(reg.values[c]);
      hasher.insert(upd_value, {reg.page_id, reg.position});
    }
  }

  auto indexed_cols = get_indexed_columns(table_metadata);
  for (auto &[c, r] : indexed_cols) {
    auto min_degree =
        calculate_btree_order(table_metadata.columns[c].max_size);
    BPTree tree(*buffer_manager, table_metadata,
                r,
                min_degree,
                table_metadata.columns[c].type,
                table_metadata.columns[c].max_size);

    for (auto &reg : result_set.registers) {
      tree.remove(reg.values[c]);
      tree.insert(upd_value, {reg.page_id, reg.position});
    }
  }

  return result_set;
}

ResultSet Megatron::update_from_fixed_page(
    serial::TableMetadata &table_metadata,
    uint32_t update_page_id,
    Comparator &comparator,
    size_t upd_col_index, SQL_type_ &upd_value) {
  auto &frame = buffer_manager->load_pin_page(update_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;
  auto page_bytes_it = page_bytes.begin();

  // Se saca metadata relevante
  auto page_header =
      serial::deserialize_page_header(page_bytes_it);
  auto fixed_data_header =
      serial::deserialize_fixed_data_header(page_bytes_it);

  ResultSet result_set;
  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
      auto register_bytes =
          get_ith_register_bytes(table_metadata, page_header,
                                 fixed_data_header, page_bytes, i);
      auto register_values =
          deserialize_register(table_metadata, register_bytes);

      if (!comparator.evaluate(register_values))
        continue;

      // Log de registro sobreescrito
      RegisterEntry reg{update_page_id,
                        static_cast<uint16_t>(i)};

      for (auto &v : register_values)
        reg.values.push_back(v);

      result_set.add_register(std::move(reg));

      // Check si registro nuevo entra en size de reg actual
      register_values[upd_col_index] = upd_value;
      register_bytes = serialize_register(table_metadata, register_values);

      if (register_bytes.size() > fixed_data_header.reg_size) {
        throw std::runtime_error("Registro modificado no entra en actual(FIXED)");
        continue;
      }

      // Sobreescribir en misma posicion

      size_t byte_offset_free_reg =
          serial::calculate_reg_offset(fixed_data_header, i);

      auto page_it = page_bytes.begin() + byte_offset_free_reg;

      // Copia registro como tal
      std::copy(register_bytes.begin(), register_bytes.end(),
                page_it);
    }
  }

  // No hubo cambio en headers, libera directamente
  buffer_manager->free_unpin_page(update_page_id, true);

  return result_set;
}

ResultSet Megatron::update_from_slotted_page(
    serial::TableMetadata &table_metadata,
    uint32_t update_page_id,
    Comparator &comparator,
    size_t upd_col_index, SQL_type_ &upd_value) {
  auto &frame = buffer_manager->load_pin_page(update_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;

  // Se saca metadata relevante
  auto page_header =
      serial::deserialize_page_header(page_bytes);
  auto slotted_data_header =
      serial::deserialize_slotted_data_header(page_bytes);

  ResultSet result_set;
  for (size_t i{}; i < slotted_data_header.n_slots; ++i) {
    if (slotted_data_header.slots[i].is_used) { // Registro existe
      auto register_bytes =
          get_ith_register_bytes(table_metadata, page_header,
                                 slotted_data_header, page_bytes, i);
      auto register_values =
          deserialize_register(table_metadata, register_bytes);

      if (!comparator.evaluate(register_values))
        continue;

      RegisterEntry reg{update_page_id,
                        static_cast<uint16_t>(i)};

      for (auto &v : register_values)
        reg.values.push_back(v);

      result_set.add_register(std::move(reg));

      // Check si registro nuevo entra en size de reg actual
      register_values[upd_col_index] = upd_value;
      register_bytes = serialize_register(table_metadata, register_values);

      if (register_bytes.size() > slotted_data_header.slots[i].reg_size) {
        std::println("Registro modificado no entra en actual, se ignora");
        continue;
      }

      size_t byte_offset_free_reg =
          serial::prepare_slotted_update(slotted_data_header,
                                         i,
                                         register_bytes.size());

      // Sobreescritura de registro en offset correcto
      auto page_it = page_bytes.begin() + byte_offset_free_reg;
      std::copy(register_bytes.begin(), register_bytes.end(),
                page_it);
    }
  }

  // Slots modificados, se tiene que escribir
  // TODO: No es necesario serializar page header
  auto page_it = page_bytes.begin();
  {
    serial::serialize_page_header(page_header, page_it);
    serial::serialize_slotted_data_header(slotted_data_header, page_it);
  }

  buffer_manager->free_unpin_page(update_page_id, true);

  return result_set;
}
