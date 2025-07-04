#include "disk_manager.hpp"
#include "megatron.hpp"
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <print>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

using namespace std;

void clearScreen() {
  std::cout << "\033[2J\033[H";
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

    std::cout << "Ingrese número de frames en buffer pool: ";
    size_t frames;
    if (!(std::cin >> frames) || frames == 0)
      throw std::invalid_argument("Numero de frames inválido");

    std::cout << "LRU o Clock?(0,1):  ";
    size_t is_clock;
    if (!(std::cin >> is_clock))
      throw std::invalid_argument("Tipo de buffer invalido");

    load_disk(disk_name, frames, is_clock);

    std::cout << "Disco cargado correctamente" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // FIXME: Set nulls con managers
  }

  pauseAndReturn();
}

void Megatron::ui_new_disk() {
  clearScreen();
  try {
    std::cout << "=== Crear y Cargar Disco ===\n";

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

    std::cout << "Ingrese número de frames en buffer pool: ";
    size_t frames;
    if (!(std::cin >> frames) || frames == 0)
      throw std::invalid_argument("Numero de frames inválido");

    std::cout << "LRU o Clock?(0,1):  ";
    size_t is_clock;
    if (!(std::cin >> is_clock))
      throw std::invalid_argument("Tipo de buffer invalido");

    new_disk(disk_name, surfaces, tracks, sectors, bytes, sectors_block, frames, is_clock);

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
  select_print(table_name, cond, val);

  pauseAndReturn();
}

void Megatron::ui_select_table_condition() {
  clearScreen();
  string table_name, col_name, value, save, saved_table_name;
  cout << "Nombre de la tabla: ";
  getline(cin, table_name);
  cout << "Columna a evaluar: ";
  getline(cin, col_name);
  cout << "Valor a evaluar: ";
  getline(cin, value);
  // cout << "¿Deseas guardar el resultado? (s/n): ";
  // getline(cin, save);
  // bool save_b = (save == "s" || save == "S");
  // if (save_b) {
  //   cout << "Nombre para guardar resultados: ";
  //   getline(cin, saved_table_name);
  // }

  select_print(table_name, col_name, value);

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

void Megatron::ui_update_reg() {
  clearScreen();
  string table_name, cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;
  cout << "Nombre de la tabla: ";
  getline(cin, table_name);
  cout << "Nombre de la columna condicion: ";
  getline(cin, cmp_col_name);
  cout << "Valor de condicion: ";
  getline(cin, cmp_col_value);
  cout << "Nombre de la columna a modificar: ";
  getline(cin, upd_col_name);
  cout << "Nuevo valor: ";
  getline(cin, upd_col_value);

  update_condition(table_name, cmp_col_name, cmp_col_value, upd_col_name, upd_col_value);

  cout << "\"Modificación exitosa o columna no encontrada\"\n";
  pauseAndReturn();
}

void Megatron::ui_update_nth_reg() {
  clearScreen();
  try {
    string table_name, cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;
    cout << "Nombre de la tabla: ";
    getline(cin, table_name);

    cout << "Nombre de la columna a modificar: ";
    getline(cin, upd_col_name);

    cout << "Nuevo valor: ";
    getline(cin, upd_col_value);

    cout << "N-esimo registro a eliminar: ";
    size_t nth_reg;
    if (!(std::cin >> nth_reg))
      throw std::invalid_argument("Posicion invalida");

    update_nth_reg(table_name, nth_reg, upd_col_name, upd_col_value);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  cout << "\"Modificación exitosa o columna no encontrada\"\n";
  pauseAndReturn();
}

void Megatron::ui_delete_data() {
  clearScreen();

  try {
    string table_name, col_name, value;
    cout << "Nombre de la tabla: ";
    getline(cin, table_name);
    cout << "Columna para condición: ";
    getline(cin, col_name);
    cout << "Valor a evaluar: ";
    getline(cin, value);

    delete_condition(table_name, col_name, value);

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

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
      throw std::invalid_argument("Posicion invalida");

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

void Megatron::ui_interact_buffer_manager() {
  string table_name;
  cout << "Nombre de tabla para interpretar\n";
  getline(cin, table_name);

  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

  buffer_manager->flush_all();

  translate();

  auto page_ids = get_used_pages(table_metadata);

  buffer_manager->clear();
  buffer_manager->set_verbose(true);

  // Info de paginas correspondientes a tabla

  // buffer_ui = std::make_unique<BufferUI>(buffer_manager_ptr->pool_.capacity(),
  //                                        disk, table_metadata);

  int opcion;
  while (true) {
    translate();

    clearScreen();

    if (buffer_manager->is_buffer_clock()) {
      buffer_manager->print_buffer_clock();

    } else {
      buffer_manager->print_buffer_LRU();
      buffer_manager->print_LRU_list();
    }

    buffer_manager->print_hit_rate();

    // cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "\n\033[1m=== MENU ===\033[0m\n";
    cout << "1. Cargar pagina\n";
    cout << "2. Establecer pin fijo\n";
    cout << "3. Quitar pin fijo\n";
    cout << "4. Mostrar contenido pagina\n";
    cout << "5. Select *\n";
    cout << "6. Select from\n";
    cout << "7. Select nth registro(empieza en 0)\n"; // FIXME: Tal vez irrelevante
    cout << "8. Update de pagina(condicion)\n";
    cout << "9. Update de pagina(nth, empieza en 0)\n";
    cout << "10. Delete de pagina(condicion)\n";
    cout << "11. Delete de pagina(nth, empieza en 0)\n";
    cout << "12. Insert 1 registro a pagina(manual)\n";
    cout << "13. Insert n registros a pagina(de csv)\n";
    cout << "14. Guardar una pagina\n";
    cout << "15. Guardar TODAS las paginas\n";
    cout << "16. Clear buffer(NO GUARDA)\n";
    cout << "17. Mostrar paginas usadas por tabla\n";
    cout << "0. Salir\n";
    cout << "Opcion: ";
    cin >> opcion;

    if (opcion == 0) {
      cout << "Deseas guardar las paginas existentes en buffer(dirty) antes de salir?(0:no, 1:si) ";
      int guardar;
      cin >> guardar;

      if (!guardar)
        buffer_manager->clear();

      buffer_manager->flush_all();

      buffer_manager->set_verbose(false);

      break;
    }

    if (opcion == 1) {
      int page_id, operacion, pinea;
      cout << "ID de pagina: ";
      cin >> page_id;

      cout << "Operacion (0 = lectura, 1 = escritura): ";
      cin >> operacion;

      cout << "?Se fija pagina(ESTO CAMBIA PIN ACTUAL)?(0, 1): ";
      cin >> pinea;

      buffer_manager->load_pin_page_push_op(page_id, (operacion == 0) ? 'R' : 'W');
      buffer_manager->set_fixed_pin(page_id, pinea);

    } else if (opcion == 2) {
      int page_id;
      cout << "ID de pagina a fijar: ";
      cin >> page_id;
      buffer_manager->set_fixed_pin(page_id, true);

    } else if (opcion == 3) {
      int page_id;
      cout << "ID de pagina a desfijar: ";
      cin >> page_id;
      buffer_manager->set_fixed_pin(page_id, false);

    } else if (opcion == 4) {
      int page_id;
      cout << "ID de pagina a mostrar: ";
      cin >> page_id;

      auto page_bytes = buffer_manager->get_page_bytes(page_id);

      if (page_bytes.empty())
        cout << "Pagina no cargada\n";
      else {
        std::cout << "\n"
                  << translate_data_page_no_write(table_metadata, page_bytes, page_id) << std::endl;
      }

    } else if (opcion == 5) {
      std::string cond = "", val = "";
      size_t page_limit{};
      // cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      cout << "# de paginas maximas a cargar: ";
      cin >> page_limit;

      select_print(table_metadata, cond, val,
                   page_limit);

    } else if (opcion == 6) {
      cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      string col_name, value;
      cout << "Columna a evaluar: ";
      getline(cin, col_name);
      cout << "Valor a evaluar: ";
      getline(cin, value);
      size_t page_limit{}, nth{};
      cout << "# de paginas maximas a cargar: ";
      cin >> page_limit;

      select_print(table_metadata, col_name,
                   value, page_limit);

    } else if (opcion == 7) {
      cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      string col_name, value;
      cout << "Columna a evaluar: ";
      getline(cin, col_name);
      cout << "Valor a evaluar: ";
      getline(cin, value);

      size_t page_limit{}, nth{};
      cout << "N-esimo registro(EXISTENTE) a seleccionar(empieza 0): ";
      cin >> nth;
      cout << "# de paginas maximas a cargar: ";
      cin >> page_limit;

      // TODO: Implementar

      // select(table_metadata, col_name,
      // value, page_limit);
    } else if (opcion == 8) { // Update condicion
      int page_id;
      cout << "ID de pagina a modificar: ";
      cin >> page_id;

      cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      string cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;
      cout << "Nombre de la columna condicion: ";
      getline(cin, cmp_col_name);
      cout << "Valor de condicion: ";
      getline(cin, cmp_col_value);
      cout << "Nombre de la columna a modificar: ";
      getline(cin, upd_col_name);
      cout << "Nuevo valor: ";
      getline(cin, upd_col_value);

      auto result_set =
          update_from_page(table_metadata, page_id,
                           cmp_col_name, cmp_col_value,
                           upd_col_name, upd_col_value);

      std::println("Se modifico los registros: ");
      for (auto &r : result_set) {
        std::println("{}", r);
      }

    } else if (opcion == 9) { // Update nth
      int page_id;
      cout << "ID de pagina a modificar: ";
      cin >> page_id;

      cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      string cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;

      cout << "Nombre de la columna a modificar: ";
      getline(cin, upd_col_name);

      cout << "Nuevo valor: ";
      getline(cin, upd_col_value);

      cout << "N-esimo registro(EXISTENTE) a modificar(empieza en 0): ";
      size_t nth_reg;
      std::cin >> nth_reg;

      auto result_set =
          update_nth_from_page(table_metadata, page_id, nth_reg,
                               upd_col_name, upd_col_value);

      std::println("Se modifico el registro: ");
      for (auto &r : result_set) {
        std::println("{}", r);
      }
    } else if (opcion == 10) { // Eliminar condicion
      int page_id;
      cout << "ID de pagina a eliminar: ";
      cin >> page_id;

      cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      string cmp_col_name, cmp_col_value;
      cout << "Nombre de la columna condicion: ";
      getline(cin, cmp_col_name);
      cout << "Valor de condicion: ";
      getline(cin, cmp_col_value);

      auto result_set =
          delete_from_page(table_metadata, page_id, cmp_col_name, cmp_col_value);

      std::println("Se modifico los registros: ");
      for (auto &r : result_set) {
        std::println("{}", r);
      }

    } else if (opcion == 11) { // Eliminar nth
      int page_id;
      cout << "ID de pagina a modificar: ";
      cin >> page_id;

      cout << "N-esimo registro(EXISTENTE) a eliminar(empieza en 0): ";
      size_t nth_reg;
      std::cin >> nth_reg;

      auto result_set =
          delete_nth_from_page(table_metadata, page_id,
                               nth_reg);

      std::println("Se modifico el registro: ");
      for (auto &r : result_set) {
        std::println("{}", r);
      }
    } else if (opcion == 12) { // Insert 1 reg
      try {
        int page_id;
        cout << "ID de pagina a insertar: ";
        cin >> page_id;

        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        string reg_line, token;
        cout << "Ingrese registro en formato csv: \n";
        getline(cin, reg_line);

        std::istringstream line_ss(reg_line);

        // Columnas
        std::vector<std::string> reg_values;
        while (std::getline(line_ss, token, ','))
          reg_values.push_back(token);

        if (reg_values.size() != table_metadata.columns.size())
          reg_values.resize(table_metadata.columns.size());

        insert_into_page(table_metadata, page_id, reg_values);

      } catch (const std::exception &e) {
        std::cerr << "\nError: " << e.what() << "\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }

    } else if (opcion == 13) { // Load n de csv
      try {
        int page_id;
        cout << "ID de pagina a insertar: ";
        cin >> page_id;

        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        string path;
        cout << "Path de csv: \n";
        getline(cin, path);

        int n_regs;
        cout << "Nregs a insertar: ";
        cin >> n_regs;

        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::ifstream file(path);
        if (file.is_open()) {
          std::string line, token;
          std::istringstream line_ss;

          /*
           * Insercion de tuplas(csv) a files
           */
          size_t records_inserted = 0;
          while (std::getline(file, line)) {

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

            insert_into_page(table_metadata,
                             page_id, reg_values);

            records_inserted++;
          }

        } else {
          std::cerr << "Archivo no existente" << std::endl;
        }
      } catch (const std::exception &e) {
        std::cerr << "\nError: " << e.what() << "\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
    } else if (opcion == 14) { // FIXME:
    } else if (opcion == 15) {
      buffer_manager->flush_all();
    } else if (opcion == 16) {
      buffer_manager->clear();
    } else if (opcion == 17) {
      for (auto e : page_ids)
        std::cout << e.first << " " << e.second << '\n';
      std::cout << std::endl;
    } else {
      cout << "Opcion invalida.\n";
    }
    pauseAndReturn();
  }
  buffer_manager->set_verbose(false);
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
  cout << "8. Modificar por condicion\n";
  cout << "9. Modificar n-esimo registro\n";
  cout << "10. Eliminar condicion\n";
  cout << "11. Eliminar n-esimo registro\n";
  cout << "12. Cargar CSV\n";
  cout << "13. Cargar n datos desde CSV\n";
  cout << "14. Mostrar specs de disco\n";
  cout << "15. Mostrar metadata de tabla\n";
  cout << "16. Translate disco\n";
  cout << "17. Set #frames por buffer pool\n";
  cout << "18. Interactuar Buffer Manager\n";
  cout << "19. Mostrar hits\n";
  cout << "20. Salir\n";
  cout << "Seleccione una opción: ";
}
