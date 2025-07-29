#include "megatron.hpp"
#include <iostream>

size_t Megatron::get_column_index(serial::TableMetadata &table_metadata,
                                  std::string &col_name) {
  auto &columns = table_metadata.columns;

  for (size_t i{}; i < columns.size(); ++i) {
    if (array_to_string_view(columns[i].name) == col_name)
      return i;
  }

  return table_metadata.n_cols;
}

void Megatron::show_table_metadata(std::string &table_name) {
  serial::TableMetadata table_metadata;
  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

  auto structures = translate();

  // Metadata Basica
  std::cout << std::format("Tabla ubicada en bloque: {}\n"
                           "Nombre: {}\n"
                           "Registros fijos?: {}\n"
                           "Tamanio maximo de registro: {}\n"
                           "Numero de columnas: {}",
                           table_metadata.table_block_id, array_to_string_view(table_metadata.name),
                           table_metadata.are_regs_fixed, table_metadata.max_reg_size,
                           table_metadata.columns.size())
            << std::endl;

  // Columnas
  for (auto &col : table_metadata.columns) {
    std::cout << std::format("({}, {}, {})", array_to_string_view(col.name), col.type, col.max_size) << " ";
  }
  std::cout << "\n Listado de bloques usados por tabla:" << std::endl;

  // Se muestra todos los bloques usados
  //  Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id, total_size{}, n_regs{};

  std::vector<unsigned char> page_bytes;
  while (curr_page_id != disk_manager->NULL_BLOCK) {
    disk_manager->read_block(page_bytes, curr_page_id);
    std::span<unsigned char> page_data(page_bytes);

    // Se lee PageHeader para contar registros
    serial::PageHeader page_header(page_data);

    // page_header = serial::deserialize_page_header(page_bytes_it);

    total_size += disk_manager->BLOCK_SIZE - page_header.free_space;
    n_regs += page_header.n_regs;

    std::cout << " -> " << curr_page_id;
    curr_page_id = page_header.next_block_id;
  }

  std::cout << "\nPaginas con directorios/buckets: \n";
  for (auto h : structures.first)
    std::cout << h << ", ";

  std::cout << "Paginas con nodos B+: \n";
  for (auto n : structures.second)
    std::cout << n << ", ";

  std::cout << "\nN_registros: " << n_regs << "\nBytes usados: " << total_size << std::endl;
}
