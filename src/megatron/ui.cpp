#include "disk_manager.hpp"
#include "megatron.hpp"
#include "ui/utils.hpp"
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
      buf_load_page();
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
      buf_show_page_content(table_metadata);
    } else if (opcion == 5) {
      buf_select_all(table_metadata);
    } else if (opcion == 6) {
      buf_select_condition(table_metadata);
    } else if (opcion == 8) {
      buf_update_condition(table_metadata);
    } else if (opcion == 9) {
      buf_update_nth(table_metadata);
    } else if (opcion == 10) {
      buf_delete_condition(table_metadata);
    } else if (opcion == 11) {
      buf_delete_nth(table_metadata);
    } else if (opcion == 12) {
      buf_insert_line(table_metadata);
    } else if (opcion == 13) {
      buf_insert_n_csv(table_metadata);
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
