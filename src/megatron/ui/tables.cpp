#include "megatron.hpp"
#include "utils.hpp"

void Megatron::ui_new_table() {
  clearScreen();
  try {
    std::cout << "=== Crear Tabla ===\n";
    showSQLTypeTable();

    std::cout << "Formato:\n"
              << "nombretabla#nombrecolumna1#tipocolumna1#nombrecolumna2#tipocolumna2..." << std::endl;

    std::string table_line;
    std::getline(std::cin, table_line);

    std::stringstream ss(table_line);
    std::string item;
    std::vector<std::string> curr_part;

    // Dividir la línea por '#'
    while (std::getline(ss, item, '#')) {
      curr_part.push_back(item);
    }

    // Minimo una columna
    if (curr_part.size() < 3 || (curr_part.size() - 1) % 2 != 0) {
      std::cerr << "Formato inválido.\n";
      return;
    }

    std::string table_name = curr_part[0];
    std::vector<std::pair<std::string, std::string>> columns;

    for (size_t i = 1; i < curr_part.size(); i += 2) {
      columns.emplace_back(curr_part[i], curr_part[i + 1]);
    }

    if (create_table(table_name, columns))
      std::cout << "\nTabla creada exitosamente\n";
    else
      std::cout << "\nError al crear exitosamente\n";

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  pauseAndReturn();
}

void Megatron::ui_show_table_metadata() {
  clearScreen();

  std::string table_name;
  std::cout << "Nombre de la tabla: ";
  getline(std::cin, table_name);

  show_table_metadata(table_name);

  pauseAndReturn();
}
