#include "megatron.hpp"
#include "serial/table.hpp"
#include "utils.hpp"
#include <iomanip>

void Megatron::ui_update_reg() {
  clearScreen();
  try {
    std::string table_name, cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;
    std::cout << "Nombre de la tabla: ";
    getline(std::cin, table_name);
    std::cout << "Nombre de la columna a modificar: ";
    getline(std::cin, upd_col_name);
    std::cout << "Nuevo valor: ";
    getline(std::cin, upd_col_value);

    serial::TableMetadata table_metadata;
    if (!search_table(table_name, table_metadata))
      throw std::invalid_argument("Tabla no existente");

    auto comparator = ui_generate_comparator(table_metadata);
    update_condition(table_metadata, comparator, upd_col_name, upd_col_value);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  pauseAndReturn();
}

void Megatron::ui_update_nth_reg() {
  clearScreen();
  try {
    std::string table_name, cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;
    std::cout << "Nombre de la tabla: ";
    getline(std::cin, table_name);

    std::cout << "Nombre de la columna a modificar: ";
    getline(std::cin, upd_col_name);

    std::cout << "Nuevo valor: ";
    getline(std::cin, upd_col_value);

    std::cout << "N-esimo registro a eliminar: ";
    size_t nth_reg;
    if (!(std::cin >> nth_reg))
      throw std::invalid_argument("Posicion invalida");

    serial::TableMetadata table_metadata;
    if (!search_table(table_name, table_metadata))
      throw std::invalid_argument("Tabla no existente");

    update_nth_reg(table_metadata, nth_reg, upd_col_name, upd_col_value);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  pauseAndReturn();
}
