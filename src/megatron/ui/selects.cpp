#include "comparison.hpp"
#include "megatron.hpp"
#include "utils.hpp"
#include <iomanip>

void Megatron::ui_select_table() {
  clearScreen();
  std::string table_name;
  std::cout << "Nombre de la tabla a consultar: ";
  getline(std::cin, table_name);

  Comparator comp;

  std::cout << std::dec << std::resetiosflags(std::ios_base::floatfield);
  select_print(table_name, comp);

  pauseAndReturn();
}

void Megatron::ui_select_table_condition() {
  clearScreen();
  std::string table_name, col_name, value, save, saved_table_name;
  std::cout << "Nombre de la tabla: ";
  getline(std::cin, table_name);
  // std::cout << "Columna a evaluar: ";
  // getline(std::cin, col_name);
  // std::cout << "Valor a evaluar: ";
  // getline(std::cin, value);
  // std::cout << "¿Deseas guardar el resultado? (s/n): ";
  // getline(cin, save);
  // bool save_b = (save == "s" || save == "S");
  // if (save_b) {
  //   std::cout << "Nombre para guardar resultados: ";
  //   getline(cin, saved_table_name);
  // }

  // TODO: guardar table_metadata
  auto comparator = ui_generate_comparator(table_name);

  select_print(table_name, comparator);

  pauseAndReturn();
}
