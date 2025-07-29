#include "megatron.hpp"
#include "ui/utils.hpp"

void Megatron::ui_interact_buffer_manager() {
  std::string table_name;
  std::cout << "Nombre de tabla para interpretar\n";
  getline(std::cin, table_name);

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

    // std::cin.ignore(numeric_limits<streamsize>::max(), '\n');
    std::cout << "\n\033[1m=== MENU ===\033[0m\n";
    std::cout << "1. Cargar pagina\n";
    std::cout << "2. Establecer pin fijo\n";
    std::cout << "3. Quitar pin fijo\n";
    std::cout << "4. Mostrar contenido pagina\n";
    std::cout << "5. Select *\n";
    std::cout << "6. Select from\n";
    std::cout << "7. Select nth registro(empieza en 0)\n"; // FIXME: Tal vez irrelevante
    std::cout << "8. Update de pagina(condicion)\n";
    std::cout << "9. Update de pagina(nth, empieza en 0)\n";
    std::cout << "10. Delete de pagina(condicion)\n";
    std::cout << "11. Delete de pagina(nth, empieza en 0)\n";
    std::cout << "12. Insert 1 registro a pagina(manual)\n";
    std::cout << "13. Insert n registros a pagina(de csv)\n";
    std::cout << "14. Guardar una pagina\n";
    std::cout << "15. Guardar TODAS las paginas\n";
    std::cout << "16. Clear buffer(NO GUARDA)\n";
    std::cout << "17. Mostrar paginas usadas por tabla\n";
    std::cout << "0. Salir\n";
    std::cout << "Opcion: ";
    std::cin >> opcion;

    if (opcion == 0) {
      std::cout << "Deseas guardar las paginas existentes en buffer(dirty) antes de salir?(0:no, 1:si) ";
      int guardar;
      std::cin >> guardar;

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
      std::cout << "ID de pagina a fijar: ";
      std::cin >> page_id;
      buffer_manager->set_fixed_pin(page_id, true);

    } else if (opcion == 3) {
      int page_id;
      std::cout << "ID de pagina a desfijar: ";
      std::cin >> page_id;
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
      std::cout << "Opcion invalida.\n";
    }

    pauseAndReturn();
  }
  buffer_manager->set_verbose(false);
}
