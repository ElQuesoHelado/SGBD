#include "megatron.hpp"
#include "utils.hpp"

void Megatron::ui_insert_data() {
  clearScreen();

  std::string table_name;
  std::cout << "Nombre de la tabla: ";
  getline(std::cin, table_name);

  std::string line;
  std::cout << "Ingrese la linea estilo CSV:\n";
  std::cout << "4,1,1,\"Futrelle; Mrs. Jacques Heath (Lily May Peel)\",female,35,1,0,113803,,C123,S\n";

  std::getline(std::cin, line);

  std::vector<std::string> values;
  std::string part;
  bool quoted = false;

  for (size_t i = 0; i < line.length(); ++i) {
    char c = line[i];

    if (c == '"') {
      quoted = !quoted; // alternar estado de comillas
    } else if (c == ',' && !quoted) {
      values.push_back(part);
      part.clear();
    } else {
      part += c;
    }
  }

  values.push_back(part); // último campo

  insert(table_name, values);

  std::cout << "\"Datos insertados exitosamente\"\n";
  pauseAndReturn();
}

void Megatron::ui_load_csv() {
  clearScreen();
  std::string csv_path, table_name;
  std::cout << "Nombre del archivo CSV: ";
  getline(std::cin, csv_path);
  std::cout << "Nombre de la tabla destino: ";
  getline(std::cin, table_name);

  load_CSV(csv_path, table_name);

  pauseAndReturn();
}

void Megatron::ui_load_n_regs_csv() {
  clearScreen();
  std::string csv_path, table_name, n_lines_str;
  std::cout << "Nombre del archivo CSV: ";
  getline(std::cin, csv_path);
  std::cout << "Nombre de la tabla destino: ";
  getline(std::cin, table_name);
  std::cout << "n_lineas a cargar: ";
  getline(std::cin, n_lines_str);
  size_t n_lines = stoul(n_lines_str);

  load_CSV(csv_path, table_name, n_lines);

  pauseAndReturn();
}
