#include "megatron.hpp"
#include "serial/generic.hpp"
#include "serial/table.hpp"
#include <iostream>
#include <string_view>

void Megatron::load_CSV(std::string path, std::string table_name, size_t n_regs) {
  std::ifstream file(path);
  if (file.is_open()) {
    std::string line, token;
    std::istringstream line_ss;

    serial::TableMetadata table_metadata;

    // Metadata invalida, no existe tabla
    if (!search_table(table_name, table_metadata)) {
      std::cerr << "Tabla no existe" << std::endl;
      return;
    }

    /*
     * Insercion de tuplas(csv) a files
     */
    size_t records_inserted = 0;
    while (std::getline(file, line)) {
      // std::cout << line << std::endl;

      // if (line.empty() || !file.good()) {
      //   break; // Salir si hay error o EOF real
      // }

      // std::cout << "aea" << std::endl;

      // if (records_inserted == 330)
      //   std::cout << "hola" << std::endl;

      if (n_regs > 0 && records_inserted >= n_regs) {
        break;
      }

      line_ss.clear();
      line_ss.str(line);

      // TODO: mas eficiente

      // Columnas
      std::vector<std::string> reg_values;
      while (std::getline(line_ss, token, ','))
        reg_values.push_back(token);

      if (reg_values.size() != table_metadata.columns.size())
        reg_values.resize(table_metadata.columns.size());

      if (table_metadata.are_regs_fixed)
        insert_fixed(table_metadata, reg_values);
      else
        insert_slotted(table_metadata, reg_values);

      records_inserted++;
    }

  } else {
    std::cerr << "Archivo no existente" << std::endl;
  }
}
