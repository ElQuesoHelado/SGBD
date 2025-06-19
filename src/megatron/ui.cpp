#include "disk_manager.hpp"
#include "megatron.hpp"
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

using namespace std;

// FIXME: ?La causa de problemas en sqltype?
void clearScreen() {
  system("clear");
}

void pauseAndReturn() {
  cout << "\nENTER para regresar ..." << flush;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');
  // cin.sync();
  cin.get();
}

void showInputError(const std::string &expected = "una entrada válida") {
  std::cerr << "\nError: Se esperaba " << expected << ". Intente nuevamente.\n";
  pauseAndReturn();
}

void showSQLTypeTable() {
  cout << "\nTipos disponibles:\n";
  cout << "+------------------+\n";
  cout << "| Tipo SQL         |\n";
  cout << "+------------------+\n";
  cout << "| TINYINT          |\n";
  cout << "| SMALLINT         |\n";
  cout << "| INTEGER          |\n";
  cout << "| BIGINT           |\n";
  cout << "| FLOAT            |\n";
  cout << "| DOUBLE           |\n";
  cout << "| CHAR(N)          |\n";
  cout << "| VARCHAR(N)       |\n";
  cout << "+------------------+\n";
}

void Megatron::ui_load_disk() {
  clearScreen();

  try {
    std::cout << "=== Cargar Disco ===\n";
    std::cout << "Nombre de disco a cargar: ";

    std::string disk_name;
    std::getline(std::cin, disk_name);

    load_disk(disk_name);

    std::cout << "Disco cargado correctamente" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  pauseAndReturn();
}

void Megatron::ui_new_disk() {
  clearScreen();
  try {
    std::cout << "=== Crear Disco ===\n";

    std::cout << "Nombre de disco a crear: ";

    std::string disk_name;
    std::getline(std::cin, disk_name);

    std::cout << "Ingrese número de superficies: ";
    size_t surfaces;
    if (!(std::cin >> surfaces))
      throw std::invalid_argument("Número de superficies inválido");

    std::cout << "Ingrese número de pistas por superficie: ";
    size_t tracks;
    if (!(std::cin >> tracks))
      throw std::invalid_argument("Número de pistas inválido");

    std::cout << "Ingrese número de sectores por pista: ";
    size_t sectors;
    if (!(std::cin >> sectors))
      throw std::invalid_argument("Número de sectores inválido");

    std::cout << "Ingrese número de bytes por sector: ";
    size_t bytes;
    if (!(std::cin >> bytes))
      throw std::invalid_argument("Número de bytes inválido");

    std::cout << "Ingrese número de sectores por bloque: ";
    size_t sectors_block;
    if (!(std::cin >> sectors_block))
      throw std::invalid_argument("Número de sectores por bloque invalido");

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    new_disk(disk_name, surfaces, tracks, sectors, bytes, sectors_block);

    std::cout << "\nDisco creado exitosamente\n";

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  pauseAndReturn();
}

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

void Megatron::ui_select_table() {
  clearScreen();
  string table_name;
  cout << "Nombre de la tabla a consultar: ";
  getline(cin, table_name);

  std::string cond = "", val = "";

  std::cout << std::dec << std::resetiosflags(std::ios_base::floatfield);
  select(table_name, cond, val);

  pauseAndReturn();
}

// TODO: implementar
void Megatron::ui_select_table_condition() {
  clearScreen();
  string table_name, col_name, value, save, saved_table_name;
  cout << "Nombre de la tabla: ";
  getline(cin, table_name);
  cout << "Columna a evaluar: ";
  getline(cin, col_name);
  cout << "Valor a evaluar: ";
  getline(cin, value);
  cout << "¿Deseas guardar el resultado? (s/n): ";
  getline(cin, save);
  bool save_b = (save == "s" || save == "S");
  if (save_b) {
    cout << "Nombre para guardar resultados: ";
    getline(cin, saved_table_name);
  }

  // select(table_name, col_name, value);

  pauseAndReturn();
}

void Megatron::ui_insert_data() {
  clearScreen();

  string table_name;
  cout << "Nombre de la tabla: ";
  getline(cin, table_name);

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

  cout << "\"Datos insertados exitosamente\"\n";
  pauseAndReturn();
}

// TODO: implementar
void Megatron::ui_update_reg() {
  clearScreen();
  string tabla, columna, valor;
  cout << "Nombre de la tabla: ";
  getline(cin, tabla);
  cout << "Nombre de la columna a modificar: ";
  getline(cin, columna);
  cout << "Nuevo valor: ";
  getline(cin, valor);

  cout << "\"Modificación exitosa o columna no encontrada\"\n";
  pauseAndReturn();
}

void Megatron::ui_delete_data() {
  clearScreen();
  string table_name, col_name, value;
  cout << "Nombre de la tabla: ";
  getline(cin, table_name);
  cout << "Columna para condición: ";
  getline(cin, col_name);
  cout << "Valor a evaluar: ";
  getline(cin, value);

  delete_reg(table_name, col_name, value);

  cout << "\"Registros eliminados (si existían)\"\n";
  pauseAndReturn();
}

void Megatron::ui_delete_nth() {
  clearScreen();

  try {
    string table_name;
    cout << "Nombre de la tabla: ";
    getline(cin, table_name);
    cout << "N-esimo registro a eliminar: ";
    size_t nth_reg;
    if (!(std::cin >> nth_reg))
      throw std::invalid_argument("Número de superficies inválido");

    delete_nth_reg(table_name, nth_reg);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  cout << "\"Registros eliminados (si existían)\"\n";
  pauseAndReturn();
}

void Megatron::ui_load_csv() {
  clearScreen();
  string csv_path, table_name;
  cout << "Nombre del archivo CSV: ";
  getline(cin, csv_path);
  cout << "Nombre de la tabla destino: ";
  getline(cin, table_name);

  load_CSV(csv_path, table_name);

  pauseAndReturn();
}

void Megatron::ui_load_n_regs_csv() {
  clearScreen();
  string csv_path, table_name, n_lines_str;
  cout << "Nombre del archivo CSV: ";
  getline(cin, csv_path);
  cout << "Nombre de la tabla destino: ";
  getline(cin, table_name);
  cout << "n_lineas a cargar: ";
  getline(cin, n_lines_str);
  size_t n_lines = stoul(n_lines_str);

  load_CSV(csv_path, table_name, n_lines);

  pauseAndReturn();
}

void Megatron::ui_find_reg() {
  clearScreen();

  try {
    string table_name;
    cout << "Nombre de la tabla: ";
    getline(cin, table_name);
    cout << "N-esimo registro a encontrar: ";
    size_t nth_reg;
    if (!(std::cin >> nth_reg))
      throw std::invalid_argument("Número de superficies inválido");

    find_nth_reg(table_name, nth_reg);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  cout << "\"Registros eliminados (si existían)\"\n";
  pauseAndReturn();
}

void Megatron::ui_show_table_metadata() {
  clearScreen();

  string table_name;
  cout << "Nombre de la tabla: ";
  getline(cin, table_name);

  show_table_metadata(table_name);

  pauseAndReturn();
}

void mostrarMenu() {
  cout << "=== Gestor de Base de Datos ===\n";
  cout << "1. Cargar disco\n";
  cout << "2. Crear disco\n";
  cout << "3. Crear tabla\n";
  cout << "4. Select *\n";
  cout << "5. Select con condición\n";
  cout << "6. Ubicar registro\n";
  cout << "7. Insertar registro individual\n";
  cout << "8. Modificar\n";
  cout << "9. Eliminar condicion\n";
  cout << "10. Eliminar n-esimo registro\n";
  cout << "11. Cargar CSV\n";
  cout << "12. Cargar n datos desde CSV\n";
  cout << "13. Mostrar specs de disco\n";
  cout << "14. Mostrar metadata de tabla\n";
  cout << "15. Translate disco\n";
  cout << "16. Set #frames por buffer pool\n";
  cout << "20. Salir\n";
  cout << "Seleccione una opción: ";
}
