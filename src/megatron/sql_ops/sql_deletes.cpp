#include "megatron.hpp"
#include "serial/fixed_page.hpp"
#include "serial/slotted_page.hpp"
#include <iostream>

ResultSet Megatron::delete_condition(std::string &table_name,
                                     Comparator &comparator) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }
  return delete_condition(table_metadata, comparator);
}

ResultSet Megatron::delete_condition(
    serial::TableMetadata &table_metadata,
    Comparator &comparator) {

  // Sin condicion no hay delete
  if (comparator.empty()) {
    std::cerr << "No se encontro columna para realizar un delete" << std::endl;
    return {};
  }

  ResultSet result_set{};
  result_set.add_columns(table_metadata.columns);

  size_t curr_page_id = table_metadata.first_page_id;
  while (curr_page_id != disk_manager->NULL_BLOCK) {
    auto &frame = buffer_manager->load_pin_page(curr_page_id);
    std::span<unsigned char> page_data(frame.page_bytes);

    serial::PageHeader page_header(page_data);

    // Solo se dio lectura de header, es limpio
    buffer_manager->free_unpin_page(curr_page_id, false);

    auto partial_result_set = delete_from_page(table_metadata,
                                               curr_page_id,
                                               comparator);

    result_set.merge(std::move(partial_result_set));

    curr_page_id = page_header.next_block_id;
  }

  return result_set;
}

ResultSet Megatron::delete_from_page(
    serial::TableMetadata &table_metadata,
    uint32_t delete_page_id,
    Comparator &comparator) {

  if (comparator.empty()) {
    std::cerr << "No se encontro columna para realizar un delete" << std::endl;
    return {};
  }

  auto result_set =
      (table_metadata.are_regs_fixed)
          ? delete_from_fixed_page(
                table_metadata, delete_page_id, comparator)
          : delete_from_slotted_page(
                table_metadata, delete_page_id, comparator);

  delete_set_hash(table_metadata, result_set);
  delete_set_index(table_metadata, result_set);

  return result_set;
}

ResultSet Megatron::delete_from_fixed_page(
    serial::TableMetadata &table_metadata,
    uint32_t delete_page_id, Comparator &comparator) {
  auto &frame = buffer_manager->load_pin_page(delete_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;
  std::span<unsigned char> page_data(page_bytes);

  serial::FixedPage fixed_page(page_data, page_bytes);

  // Se saca metadata relevante
  auto &page_header = fixed_page.page_header;
  auto &fixed_data_header = fixed_page.fixed_data_header;

  ResultSet result_set;
  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    if (fixed_data_header.free_register_bitmap->test(i)) { // Registro existe
      auto register_bytes =
          get_ith_register_bytes(table_metadata, page_header,
                                 fixed_data_header, page_bytes, i);
      auto register_values =
          deserialize_register(table_metadata, register_bytes);

      if (!comparator.evaluate(register_values))
        continue;

      // Registramos eliminado
      RegisterEntry reg(delete_page_id,
                        i, std::move(register_values));

      result_set.add_register(std::move(reg));

      // Si cumple condicion, delete
      // Solo marcamos como libre, ya que todo es fijo se reescribira luego
      fixed_data_header.free_bytes += fixed_data_header.reg_size;
      fixed_data_header.free_register_bitmap->set(i, false);
      page_header.free_space += fixed_data_header.reg_size;
      page_header.n_regs--;
    }
  }

  buffer_manager->free_unpin_page(delete_page_id, true);

  return result_set;
}

ResultSet Megatron::delete_from_slotted_page(
    serial::TableMetadata &table_metadata,
    uint32_t delete_page_id, Comparator &comparator) {
  auto &frame = buffer_manager->load_pin_page(delete_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;
  std::span<unsigned char> page_data(page_bytes);

  serial::SlottedPage slotted_page(page_data, page_bytes);

  // Se saca metadata relevante
  auto &page_header = slotted_page.page_header;
  auto &slotted_data_header = slotted_page.slotted_data_header;

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

      RegisterEntry reg(delete_page_id,
                        i, std::move(register_values));

      result_set.add_register(std::move(reg));

      // Cumple condicion, solo marcamos slot como disponible
      // No se actualiza free_space, este depende de un compactar
      slotted_data_header.slots[i].is_used = false;
      page_header.n_regs--;
    }
  }

  buffer_manager->free_unpin_page(delete_page_id, true);

  return result_set;
}
