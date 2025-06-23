#include "megatron.hpp"
#include "serial/table.hpp"
#include <cstdint>

std::vector<uint32_t> Megatron::locate_regs_cond(std::string &table_name, std::string &col_name, std::string &condition) {
  serial::TableMetadata table_metadata;

  std::vector<uint32_t> blocks{};

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
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

  // Si no hay condicion
  if (col_index >= table_metadata.n_cols)
    return {};

  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id, n_regs{};
  while (curr_page_id != disk_manager->NULL_BLOCK) {
    // disk.read_block(page_bytes, curr_page_id);
    auto frame = buffer_manager->load_pin_page(curr_page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;

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

        // Si hay condicion
        if (col_index < table_metadata.n_cols && register_values[col_index] != cond_val)
          continue;

        blocks.push_back(curr_page_id);
        n_regs++;
        // for (auto &v : register_values)
        //   std::cout << SQL_type_to_string(v) << " | ";
        //
        // std::cout << std::endl;
      }
    }

    buffer_manager->free_unpin_page(curr_page_id);
    curr_page_id = page_header.next_block_id;
  }
  std::cout << "Numero de registros encontrados: " << n_regs << std::endl;
  std::sort(blocks.begin(), blocks.end());
  auto it = std::unique(blocks.begin(), blocks.end());

  blocks.erase(it, blocks.end());
  return blocks;
}

// Bloque,numero
std::pair<uint32_t, uint32_t> Megatron::locate_nth_reg(std::string &table_name, size_t nth) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {disk_manager->NULL_BLOCK, 0};
  }

  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id;

  // std::vector<unsigned char> page_bytes;
  while (curr_page_id != disk_manager->NULL_BLOCK) {
    auto &frame = buffer_manager->load_pin_page(curr_page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;
    auto page_bytes_it = page_bytes.begin();

    // disk.read_block(page_bytes, curr_page_id);
    // auto page_bytes_it = page_bytes.begin();

    // Se lee PageHeader para contar registros
    serial::PageHeader page_header;

    page_header = serial::deserialize_page_header(page_bytes_it);

    // En esta pagina si esta el registro a eliminar
    if (page_header.n_regs >= nth) {
      auto page_bytes_it = page_bytes.begin();

      serial::PageHeader page_header = serial::deserialize_page_header(page_bytes_it);
      serial::FixedDataHeader fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);

      for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
        if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
          if (nth > 1) {
            nth--;
            continue;
          }

          return {curr_page_id, i};

          break;
        }
      }

      break;
    }

    buffer_manager->free_unpin_page(curr_page_id);

    nth -= page_header.n_regs;
    curr_page_id = page_header.next_block_id;
  }

  return {disk_manager->NULL_BLOCK, 0};
}

// Inserta un solo registro en tabla
uint32_t Megatron::locate_free_page(serial::TableMetadata &table_metadata) {
  return get_insertable_page(table_metadata.first_page_id,
                             table_metadata.max_reg_size);
}

// SOLO carga y modifica, no guarda
std::pair<uint32_t, std::vector<unsigned char>> Megatron::insert_reg_in_page(serial::TableMetadata &table_metadata, std::string &csv_values) {
  std::istringstream line_ss;

  line_ss.str(csv_values);

  // Columnas
  std::string token;
  std::vector<std::string> values;
  while (std::getline(line_ss, token, ','))
    values.push_back(token);

  if (values.size() != table_metadata.columns.size())
    values.resize(table_metadata.columns.size());

  if (table_metadata.columns.size() != values.size()) {
    std::cerr << "Numero de valores diferente a columnas" << std::endl;
    return {disk_manager->NULL_BLOCK, {}};
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
  // if (insert_page_id == disk.NULL_BLOCK)
  //   insert_page_id = add_new_page_to_table(table_metadata);

  // Se lee pagina y saca metadata relevante
  serial::PageHeader page_header;
  serial::FixedDataHeader fixed_data_header;

  // auto &frame = buffer_manager_ptr->get_block(insert_page_id);
  std::vector<unsigned char> insert_page_bytes;
  disk_manager->read_block(insert_page_bytes, insert_page_id);

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
  // frame.dirty = true;

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

  return {insert_page_id, insert_page_bytes};
}

uint32_t Megatron::delete_nth_reg_in_page(std::vector<unsigned char> &page_bytes, size_t nth) {
  auto page_bytes_it = page_bytes.begin();

  serial::PageHeader page_header = serial::deserialize_page_header(page_bytes_it);
  serial::FixedDataHeader fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);

  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
      if (nth > 1) {
        nth--;
        continue;
      }

      // Solo marcamos como libre, ya que todo es fijo se reescribira luego
      fixed_data_header.free_bytes += fixed_data_header.reg_size;
      fixed_data_header.free_register_bitmap[i] = false;
      page_header.free_space += fixed_data_header.reg_size;
      page_header.n_regs--;

      // Se reescribe tanto page header y fixed_data_header de pagina
      auto page_it = page_bytes.begin();
      {
        serial::serialize_page_header(page_header, page_it);
        serial::serialize_fixed_block_header(fixed_data_header, page_it);
      }

      return nth;
      break;
    }
  }

  return 0;
}
