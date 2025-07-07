#include "megatron.hpp"
#include "utils.hpp"
#include <iomanip>

void Megatron::ui_update_reg() {
  clearScreen();
  std::string table_name, cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;
  std::cout << "Nombre de la tabla: ";
  getline(std::cin, table_name);
  std::cout << "Nombre de la columna condicion: ";
  getline(std::cin, cmp_col_name);
  std::cout << "Valor de condicion: ";
  getline(std::cin, cmp_col_value);
  std::cout << "Nombre de la columna a modificar: ";
  getline(std::cin, upd_col_name);
  std::cout << "Nuevo valor: ";
  getline(std::cin, upd_col_value);

  update_condition(table_name, cmp_col_name, cmp_col_value, upd_col_name, upd_col_value);

  std::cout << "\"Modificación exitosa o columna no encontrada\"\n";
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

    update_nth_reg(table_name, nth_reg, upd_col_name, upd_col_value);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  std::cout << "\"Modificación exitosa o columna no encontrada\"\n";
  pauseAndReturn();
}
