#include "megatron.hpp"
#include "utils.hpp"

void Megatron::ui_delete_data() {
  clearScreen();
  try {
    std::string table_name, col_name, value;
    std::cout << "Nombre de la tabla: ";
    getline(std::cin, table_name);

    serial::TableMetadata table_metadata;
    if (!search_table(table_name, table_metadata))
      throw std::invalid_argument("Tabla no existente");

    auto comparator = ui_generate_comparator(table_metadata);
    delete_condition(table_metadata, comparator);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  std::cout << "\"Registros eliminados (si existían)\"\n";
  pauseAndReturn();
}

void Megatron::ui_delete_nth() {
  clearScreen();

  try {
    std::string table_name;
    std::cout << "Nombre de la tabla: ";
    getline(std::cin, table_name);
    std::cout << "N-esimo registro a eliminar: ";
    size_t nth_reg;
    if (!(std::cin >> nth_reg))
      throw std::invalid_argument("Posicion invalida");

    delete_nth_reg(table_name, nth_reg);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  std::cout << "\"Registros eliminados (si existían)\"\n";
  pauseAndReturn();
}
