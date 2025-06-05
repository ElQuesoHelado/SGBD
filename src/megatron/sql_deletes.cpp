#include "megatron.hpp"

void Megatron::delete_reg(std::string &table_name, std::string &col_name, std::string &condition) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

  // Se parsea column index y condicion a SQL_type
  size_t col_index{table_metadata.n_cols};
  SQL_type cond_val;

  for (size_t i{}; i < table_metadata.columns.size(); ++i) {
    if (col_name == array_to_string_view(table_metadata.columns[i].name)) {
      col_index = i;
      cond_val = string_to_sql_type(condition, table_metadata.columns[i].type, table_metadata.columns[i].max_size);
    }
  }

  // Sin condicion no hay delete
  if (col_index >= table_metadata.n_cols) {
    std::cerr << "No se encontro columna para realizar un delete" << std::endl;
    return;
  }

  if (table_metadata.are_regs_fixed)
    delete_fixed(table_metadata, col_index, cond_val);
  else
    delete_slotted(table_metadata, col_index, cond_val);
}

void Megatron::delete_fixed(serial::TableMetadata &table_metadata, size_t col_index, SQL_type &cond_val) {
  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id;
  while (curr_page_id != disk.NULL_BLOCK) {
    std::vector<unsigned char> page_bytes;
    disk.read_block(page_bytes, curr_page_id);
    auto page_bytes_it = page_bytes.begin();

    // Se saca metadata relevante
    serial::PageHeader page_header;
    serial::FixedDataHeader fixed_data_header;
    serial::SlottedDataHeader slotted_data_header;

    page_header = serial::deserialize_page_header(page_bytes_it);

    fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);
    for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
      if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
        auto register_bytes = get_ith_register_bytes(table_metadata, page_header, fixed_data_header, page_bytes, i);
        auto register_values = deserialize_register(table_metadata, register_bytes);

        if (register_values[col_index] != cond_val)
          continue;

        // Si cumple condicion, delete
        // Solo marcamos como libre, ya que todo es fijo se reescribira luego
        fixed_data_header.free_bytes += fixed_data_header.reg_size;
        fixed_data_header.free_register_bitmap[i] = false;
        page_header.free_space += fixed_data_header.reg_size;
        page_header.n_regs--;
      }
    }

    // Se reescribe tanto page header y fixed_data_header de pagina
    auto page_it = page_bytes.begin();
    {
      serial::serialize_page_header(page_header, page_it);
      serial::serialize_fixed_block_header(fixed_data_header, page_it);
    }

    disk.write_block(page_bytes, curr_page_id);

    curr_page_id = page_header.next_block_id;
  }
}

void Megatron::delete_slotted(serial::TableMetadata &table_metadata, size_t col_index, SQL_type &cond_val) {
  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id;
  while (curr_page_id != disk.NULL_BLOCK) {
    std::vector<unsigned char> page_bytes;
    disk.read_block(page_bytes, curr_page_id);
    auto page_bytes_it = page_bytes.begin();

    // Se saca metadata relevante
    serial::PageHeader page_header;
    serial::FixedDataHeader fixed_data_header;
    serial::SlottedDataHeader slotted_data_header;

    page_header = serial::deserialize_page_header(page_bytes_it);

    slotted_data_header = serial::deserialize_slotted_data_header(page_bytes_it);
    for (size_t i{}; i < slotted_data_header.n_slots; ++i) {
      if (slotted_data_header.slots[i].is_used) { // Registro existe
        auto register_bytes = get_ith_register_bytes(table_metadata, page_header, slotted_data_header, page_bytes, i);
        auto register_values = deserialize_register(table_metadata, register_bytes);

        if (register_values[col_index] != cond_val)
          continue;

        // Cumple condicion, solo marcamos slot como disponible
        // No se actualiza free_space, este depende de un compactar
        slotted_data_header.slots[i].is_used = false;
        page_header.n_regs--;
      }
    }

    // Reemplazamos headers modificados
    auto page_it = page_bytes.begin();
    {
      serial::serialize_page_header(page_header, page_it);
      serial::serialize_slotted_data_header(slotted_data_header, page_it);
    }

    disk.write_block(page_bytes, curr_page_id);
    curr_page_id = page_header.next_block_id;
  }
}
