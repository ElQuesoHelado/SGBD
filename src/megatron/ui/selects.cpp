#include "megatron.hpp"
#include "utils.hpp"

void Megatron::ui_select_table() {
  clearScreen();
  try {
    std::string table_name;
    std::cout << "Nombre de la tabla a consultar: ";
    getline(std::cin, table_name);

    serial::TableMetadata table_metadata;
    if (!search_table(table_name, table_metadata))
      throw std::invalid_argument("Tabla no existente");

    Comparator comp;

    // std::cout << std::dec << std::resetiosflags(std::ios_base::floatfield);
    select_print(table_name, comp);
  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  pauseAndReturn();
}

void Megatron::ui_select_table_condition() {
  clearScreen();
  try {
    std::string table_name, col_name, value, save, saved_table_name;
    std::cout << "Nombre de la tabla: ";
    getline(std::cin, table_name);

    serial::TableMetadata table_metadata;
    if (!search_table(table_name, table_metadata))
      throw std::invalid_argument("Tabla no existente");

    auto comparator = ui_generate_comparator(table_metadata);
    select_print(table_metadata, comparator);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  pauseAndReturn();
}
