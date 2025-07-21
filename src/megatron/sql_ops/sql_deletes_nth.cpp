#include "megatron.hpp"
#include "result_set.hpp"
#include "serial/slotted_data.hpp"
#include <cstddef>

ResultSet Megatron::delete_nth_reg(std::string &table_name, size_t nth) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }

  return delete_nth_reg(table_metadata, nth);
}

ResultSet Megatron::delete_nth_reg(
    serial::TableMetadata &table_metadata, size_t nth) {
  ResultSet result_set{};
  result_set.add_columns(table_metadata.columns);

  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id;

  // std::vector<unsigned char> page_bytes;
  while (curr_page_id != disk_manager->NULL_BLOCK) {
    auto &frame = buffer_manager->load_pin_page(curr_page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;

    // Se lee PageHeader para contar registros
    auto page_header =
        serial::deserialize_page_header(page_bytes);

    buffer_manager->free_unpin_page(curr_page_id, false);

    // En esta pagina si esta el registro a eliminar
    if (page_header.n_regs > nth) {
      result_set =
          delete_nth_from_page(table_metadata, curr_page_id, nth);

      break;
    }

    nth -= page_header.n_regs;
    curr_page_id = page_header.next_block_id;
  }

  return result_set;
}

ResultSet Megatron::delete_nth_from_page(serial::TableMetadata &table_metadata,
                                         uint32_t delete_page_id, size_t nth) {
  auto result_set =
      (table_metadata.are_regs_fixed)
          ? delete_nth_from_fixed_page(table_metadata, delete_page_id, nth)
          : delete_nth_from_slotted_page(table_metadata, delete_page_id, nth);

  return result_set;
}

ResultSet Megatron::delete_nth_from_fixed_page(
    serial::TableMetadata &table_metadata,
    uint32_t delete_page_id, size_t nth) {
  auto &frame = buffer_manager->load_pin_page(delete_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;
  auto page_bytes_it = page_bytes.begin();

  auto page_header =
      serial::deserialize_page_header(page_bytes_it);
  auto fixed_data_header =
      serial::deserialize_fixed_data_header(page_bytes_it);

  ResultSet result_set;
  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
      if (nth > 0) {
        nth--;
        continue;
      }
      auto register_bytes =
          get_ith_register_bytes(table_metadata, page_header,
                                 fixed_data_header, page_bytes, i);
      auto register_values =
          deserialize_register(table_metadata, register_bytes);

      RegisterEntry reg{delete_page_id, i};
      for (auto &v : register_values)
        reg.values.push_back(v);

      result_set.add_register(std::move(reg));

      // Solo marcamos como libre, ya que todo es fijo se reescribira luego
      fixed_data_header.free_bytes += fixed_data_header.reg_size;
      fixed_data_header.free_register_bitmap[i] = false;
      page_header.free_space += fixed_data_header.reg_size;
      page_header.n_regs--;
      break;
    }
  }

  // Se reescribe tanto page header y fixed_data_header de pagina
  auto page_it = page_bytes.begin();
  {
    serial::serialize_page_header(page_header, page_it);
    serial::serialize_fixed_block_header(fixed_data_header, page_it);
  }

  buffer_manager->free_unpin_page(delete_page_id, true);

  return result_set;
}

ResultSet Megatron::delete_nth_from_slotted_page(serial::TableMetadata &table_metadata, uint32_t delete_page_id, size_t nth) {
  auto &frame = buffer_manager->load_pin_page(delete_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;
  auto page_bytes_it = page_bytes.begin();

  auto page_header = serial::deserialize_page_header(page_bytes_it);
  auto slotted_data_header = serial::deserialize_slotted_data_header(page_bytes_it);

  ResultSet result_set;
  for (size_t i{}; i < slotted_data_header.n_slots; ++i) {
    if (slotted_data_header.slots[i].is_used) { // Registro existe
      if (nth > 0) {
        nth--;
        continue;
      }

      auto register_bytes = get_ith_register_bytes(table_metadata,
                                                   page_header,
                                                   slotted_data_header, page_bytes, i);
      auto register_values = deserialize_register(table_metadata, register_bytes);

      RegisterEntry reg{delete_page_id, i};
      for (auto &v : register_values)
        reg.values.push_back(v);

      result_set.add_register(std::move(reg));

      // Cumple condicion, solo marcamos slot como disponible
      // No se actualiza free_space, este depende de un compactar
      slotted_data_header.slots[i].is_used = false;
      page_header.n_regs--;
      break;
    }
  }

  // Reemplazamos headers modificados
  auto page_it = page_bytes.begin();
  {
    serial::serialize_page_header(page_header, page_it);
    serial::serialize_slotted_data_header(slotted_data_header, page_it);
  }

  buffer_manager->free_unpin_page(delete_page_id, true);

  return result_set;
}
