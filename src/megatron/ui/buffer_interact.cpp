#include "megatron.hpp"
#include "serial/table.hpp"
#include <print>

void Megatron::buf_load_page() {
  int page_id, operacion, pinea;
  std::cout << "ID de pagina: ";
  std::cin >> page_id;

  std::cout << "Operacion (0 = lectura, 1 = escritura): ";
  std::cin >> operacion;

  std::cout << "?Se fija pagina(ESTO CAMBIA PIN ACTUAL)?(0, 1): ";
  std::cin >> pinea;

  buffer_manager->load_pin_page_push_op(page_id, (operacion == 0) ? 'R' : 'W');
  buffer_manager->set_fixed_pin(page_id, pinea);
}

void Megatron::buf_show_page_content(serial::TableMetadata &table_metadata) {
  int page_id;
  std::cout << "ID de pagina a mostrar: ";
  std::cin >> page_id;

  auto page_bytes = buffer_manager->get_page_bytes(page_id);

  if (page_bytes.empty())
    std::cout << "Pagina no cargada\n";
  else {
    std::cout << "\n"
              << translate_data_page_no_write(table_metadata, page_bytes, page_id) << std::endl;
  }
}

void Megatron::buf_select_all(serial::TableMetadata &table_metadata) {
  std::string cond = "", val = "";
  size_t page_limit{};
  // cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::cout << "# de paginas maximas a cargar: ";
  std::cin >> page_limit;

  // select_print(table_metadata, cond, val,
  //              page_limit);
}

void Megatron::buf_select_condition(serial::TableMetadata &table_metadata) {
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::string col_name, val;
  std::cout << "Columna a evaluar: ";
  getline(std::cin, col_name);
  std::cout << "Valor a evaluar: ";
  getline(std::cin, val);

  size_t page_limit{}, nth{};
  std::cout << "# de paginas maximas a cargar: ";
  std::cin >> page_limit;

  auto comparator = ui_generate_comparator(table_metadata);

  select_print(table_metadata, comparator, page_limit);
}

void Megatron::buf_update_condition(serial::TableMetadata &table_metadata) {
  int page_id;
  std::cout << "ID de pagina a modificar: ";
  std::cin >> page_id;

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::string cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;
  std::cout << "Nombre de la columna condicion: ";
  getline(std::cin, cmp_col_name);
  std::cout << "Valor de condicion: ";
  getline(std::cin, cmp_col_value);
  std::cout << "Nombre de la columna a modificar: ";
  getline(std::cin, upd_col_name);
  std::cout << "Nuevo valor: ";
  getline(std::cin, upd_col_value);

  auto result_set =
      update_from_page(table_metadata, page_id,
                       cmp_col_name, cmp_col_value,
                       upd_col_name, upd_col_value);

  std::println("Se modifico los registros: ");
  for (auto &r : result_set) {
    std::println("{}", r);
  }
}

void Megatron::buf_update_nth(serial::TableMetadata &table_metadata) {
  int page_id;
  std::cout << "ID de pagina a modificar: ";
  std::cin >> page_id;

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::string cmp_col_name, cmp_col_value, upd_col_name, upd_col_value;

  std::cout << "Nombre de la columna a modificar: ";
  getline(std::cin, upd_col_name);

  std::cout << "Nuevo valor: ";
  getline(std::cin, upd_col_value);

  std::cout << "N-esimo registro(EXISTENTE) a modificar(empieza en 0): ";
  size_t nth_reg;
  std::cin >> nth_reg;

  auto result_set =
      update_nth_from_page(table_metadata, page_id, nth_reg,
                           upd_col_name, upd_col_value);

  std::println("Se modifico el registro: ");
  for (auto &r : result_set) {
    std::println("{}", r);
  }
}

void Megatron::buf_delete_condition(serial::TableMetadata &table_metadata) {
  int page_id;
  std::cout << "ID de pagina a eliminar: ";
  std::cin >> page_id;

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::string cmp_col_name, cmp_col_value;
  std::cout << "Nombre de la columna condicion: ";
  getline(std::cin, cmp_col_name);
  std::cout << "Valor de condicion: ";
  getline(std::cin, cmp_col_value);

  auto result_set =
      delete_from_page(table_metadata, page_id, cmp_col_name, cmp_col_value);

  std::println("Se modifico los registros: ");
  for (auto &r : result_set) {
    std::println("{}", r);
  }
}

void Megatron::buf_delete_nth(serial::TableMetadata &table_metadata) {
  int page_id;
  std::cout << "ID de pagina a modificar: ";
  std::cin >> page_id;

  std::cout << "N-esimo registro(EXISTENTE) a eliminar(empieza en 0): ";
  size_t nth_reg;
  std::cin >> nth_reg;

  auto result_set =
      delete_nth_from_page(table_metadata, page_id,
                           nth_reg);

  std::println("Se modifico el registro: ");
  for (auto &r : result_set) {
    std::println("{}", r);
  }
}

void Megatron::buf_insert_line(serial::TableMetadata &table_metadata) {
  try {
    int page_id;
    std::cout << "ID de pagina a insertar: ";
    std::cin >> page_id;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string reg_line, token;
    std::cout << "Ingrese registro en formato csv: \n";
    getline(std::cin, reg_line);

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
}

void Megatron::buf_insert_n_csv(serial::TableMetadata &table_metadata) {
  try {
    int page_id;
    std::cout << "ID de pagina a insertar: ";
    std::cin >> page_id;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string path;
    std::cout << "Path de csv: \n";
    getline(std::cin, path);

    int n_regs;
    std::cout << "Nregs a insertar: ";
    std::cin >> n_regs;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

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
}
