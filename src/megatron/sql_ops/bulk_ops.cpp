#include "megatron.hpp"
#include "serial/table.hpp"
#include <iostream>
#include <print>

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
      // std::println("{}", records_inserted);

      if (n_regs > 0 && records_inserted >= n_regs) {
        break;
      }

      line_ss.clear();
      line_ss.str(line);

      // Columnas
      std::vector<std::string> reg_values;
      while (std::getline(line_ss, token, ','))
        reg_values.push_back(token);

      if (reg_values.size() != table_metadata.columns.size())
        reg_values.resize(table_metadata.columns.size());

      insert(table_metadata, reg_values);

      records_inserted++;
    }

  } else {
    std::cerr << "Archivo no existente" << std::endl;
  }
}
