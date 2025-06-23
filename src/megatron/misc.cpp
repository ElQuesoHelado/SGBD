#include "megatron.hpp"
#include "serial/generic.hpp"
#include <format>

void Megatron::find_nth_reg(std::string &table_name, size_t nth) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

  // Se iteran por todas las paginas
  size_t curr_page_id = table_metadata.first_page_id;

  std::vector<unsigned char> page_bytes;
  while (curr_page_id != disk_manager->NULL_BLOCK) {
    disk_manager->read_block(page_bytes, curr_page_id);
    auto page_bytes_it = page_bytes.begin();

    // Se lee PageHeader para contar registros
    serial::PageHeader page_header;

    page_header = serial::deserialize_page_header(page_bytes_it);

    // En esta pagina si esta el registro
    if (page_header.n_regs >= nth) {
      break;
    }

    nth -= page_header.n_regs;
    curr_page_id = page_header.next_block_id;
  }

  std::cout << "Registro en bloque: " << curr_page_id << "\nNumero: " << nth << std::endl;
}

void Megatron::show_table_metadata(std::string &table_name) {
  serial::TableMetadata table_metadata;
  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

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
    auto page_bytes_it = page_bytes.begin();

    // Se lee PageHeader para contar registros
    serial::PageHeader page_header;

    page_header = serial::deserialize_page_header(page_bytes_it);

    total_size += disk_manager->BLOCK_SIZE - page_header.free_space;
    n_regs += page_header.n_regs;

    std::cout << " -> " << curr_page_id;
    curr_page_id = page_header.next_block_id;
  }

  std::cout << "\nN_registros: " << n_regs << "\nBytes usados: " << total_size << std::endl;
  // Se itera por todas sus paginas para hallar n_regs y bytes used
}
