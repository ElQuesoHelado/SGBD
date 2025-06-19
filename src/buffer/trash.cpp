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
  while (curr_page_id != disk.NULL_BLOCK) {
    // disk.read_block(page_bytes, curr_page_id);
    auto frame = buffer_manager_ptr->get_block(curr_page_id);
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

    curr_page_id = page_header.next_block_id;
  }
  std::cout << "Numero de registros encontrados: " << n_regs << std::endl;
  return blocks;
}

// Bloque,numero
std::pair<uint32_t, uint32_t> Megatron::locate_nth_reg(std::string &table_name, size_t nth) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {disk.NULL_BLOCK, 0};
  }

  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id;

  // std::vector<unsigned char> page_bytes;
  while (curr_page_id != disk.NULL_BLOCK) {
    auto &frame = buffer_manager_ptr->get_block(curr_page_id);
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

    nth -= page_header.n_regs;
    curr_page_id = page_header.next_block_id;
  }

  return {disk.NULL_BLOCK, 0};
}

void Megatron::show_block(serial::TableMetadata &table_metadata, uint32_t block_id) {
  // Se itera por toda pagina de tabla
  uint32_t curr_page_id = block_id;

  std::vector<unsigned char> page_bytes;
  disk.read_block(page_bytes, curr_page_id);

  auto page_bytes_it = page_bytes.begin();

  auto page_header = serial::deserialize_page_header(page_bytes_it);
  // if (page_header.n_regs == 0) // Pagina vacia
  //   continue;

  serial::FixedDataHeader fixed_data_header;
  fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);

  show_fixed_page(table_metadata, page_header,
                  fixed_data_header, page_bytes, curr_page_id);

  // curr_page_id = page_header.next_block_id;
  // std::cout << curr_page_id << std::endl;
}

// Es una pagina completa, implica varios sectores, el primero es afectado por el header
void Megatron::show_fixed_page(
    serial::TableMetadata &table_metadata,
    serial::PageHeader &page_header,
    serial::FixedDataHeader &fixed_data_header,
    std::vector<unsigned char> &page_bytes, uint32_t curr_page_id) {

  int remm_sector_bytes =
      disk.SECTOR_SIZE -
      serial::calculate_fixed_data_header_size(fixed_data_header); // Deberia restarse size de header
  size_t ith_sector_in_block{};

  // PageHeader
  std::string out_str{};
  out_str += "Next_page_id: " + std::to_string(page_header.next_block_id) + " ";
  out_str += "N_registers: " + std::to_string(page_header.n_regs) + " ";
  out_str += "Free_space/capacity: " + std::to_string(page_header.free_space) +
             '/' + std::to_string(disk.BLOCK_SIZE - serial::calculate_fixed_data_header_size(fixed_data_header)) +
             "\n";

  // FixedDataHeader
  out_str += "Register size: " + std::to_string(fixed_data_header.reg_size) + " ";
  out_str += "Max registers: " + std::to_string(fixed_data_header.max_n_regs) + " ";
  std::string bitmap_str;
  boost::to_string(fixed_data_header.free_register_bitmap, bitmap_str);
  out_str += "Free_register_bitmap: " + bitmap_str + "\n";

  std::cout << out_str << std::endl;
  std::cout << disk.logic_sector_to_CHS(
                   disk.free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block))
            << std::endl;

  // disk.write_block_txt(out_str, curr_page_id);
  // disk.write_block_txt(disk.logic_sector_to_CHS(
  //                          disk.free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block)),
  //                      curr_page_id);

  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    out_str = "";
    if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
      auto register_bytes = get_ith_register_bytes(table_metadata,
                                                   page_header,
                                                   fixed_data_header, page_bytes, i);
      auto register_values = deserialize_register(table_metadata, register_bytes);

      for (auto &v : register_values)
        // disk.write
        out_str += SQL_type_to_string(v) + " ";

      // out_str+='\n';
    } else { // Registro vacio/deleted
      out_str += '\n';
    }
    remm_sector_bytes -= fixed_data_header.reg_size;
    if (remm_sector_bytes < 0) {
      remm_sector_bytes = disk.SECTOR_SIZE;
      ith_sector_in_block++;
      std::cout << "\n" + disk.logic_sector_to_CHS(
                              disk.free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block))
                << std::endl;
    }

    cout << out_str << std::endl;
    // disk.write_sector_txt(
    //     out_str,
    //     disk.free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block));
    // disk.write_block_txt(out_str, curr_page_id);
  }
}

// void Megatron::delete_nth_fixed(std::vector<unsigned char> &page_bytes, size_t nth) {
//   auto page_bytes_it = page_bytes.begin();
//
//   serial::PageHeader page_header = serial::deserialize_page_header(page_bytes_it);
//   serial::FixedDataHeader fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);
//
//   for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
//     if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
//       if (nth > 1) {
//         nth--;
//         continue;
//       }
//
//       // Solo marcamos como libre, ya que todo es fijo se reescribira luego
//       fixed_data_header.free_bytes += fixed_data_header.reg_size;
//       fixed_data_header.free_register_bitmap[i] = false;
//       page_header.free_space += fixed_data_header.reg_size;
//       page_header.n_regs--;
//       break;
//     }
//   }
//
//   // Se reescribe tanto page header y fixed_data_header de pagina
//   auto page_it = page_bytes.begin();
//   {
//     serial::serialize_page_header(page_header, page_it);
//     serial::serialize_fixed_block_header(fixed_data_header, page_it);
//   }
// }
