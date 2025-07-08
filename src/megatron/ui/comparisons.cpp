#include "megatron.hpp"
#include "serial/generic.hpp"
#include "types.hpp"
#include <cstddef>
#include <print>

//(nombre columna, operacion de comparacion, valor)
//("", AND/OR, "")
// TODO: throws para conversion de tipos
Comparator Megatron::generate_comparator(serial::TableMetadata &table_metadata,
                                         std::vector<std::tuple<std::string,
                                                                std::string,
                                                                std::string>>
                                             &comparisons) {
  Comparator comparator;

  for (auto &[col_name,
              op,
              value] : comparisons) {
    if (op == "AND") {
      comparator.AND();
      continue;

    } else if (op == "OR") {
      comparator.OR();
      continue;
    }

    auto col_index = get_column_index(table_metadata, col_name);
    if (col_index == table_metadata.n_cols)
      return {};
    auto sql_value =
        string_to_sql_type(value,
                           table_metadata.columns[col_index].type,
                           table_metadata.columns[col_index].max_size);

    if (op == "<") {
      comparator.less_than(sql_value, col_index);
    } else if (op == "<=") {
      comparator.less_equal_than(sql_value, col_index);
    } else if (op == ">") {
      comparator.greater_than(sql_value, col_index);
    } else if (op == ">=") {
      comparator.greater_equal_than(sql_value, col_index);
    } else {
      comparator.equals(sql_value, col_index);
    }
  }

  return comparator;
}

//(nombre columna, operacion de comparacion, valor)
//("", AND/OR, "")
// TODO: throws para conversion de tipos
Comparator Megatron::generate_comparator(
    serial::TableMetadata &table_metadata,
    std::vector<std::tuple<size_t,
                           std::string,
                           SQL_type>>
        &comparisons) {
  Comparator comparator;

  for (auto &[col_index,
              op,
              sql_value] : comparisons) {
    if (op == "AND") {
      comparator.AND();
      continue;

    } else if (op == "OR") {
      comparator.OR();
      continue;
    }

    if (col_index == table_metadata.n_cols)
      return {};

    if (op == "<") {
      comparator.less_than(sql_value, col_index);
    } else if (op == "<=") {
      comparator.less_equal_than(sql_value, col_index);
    } else if (op == ">") {
      comparator.greater_than(sql_value, col_index);
    } else if (op == ">=") {
      comparator.greater_equal_than(sql_value, col_index);
    } else {
      comparator.equals(sql_value, col_index);
    }
  }

  return comparator;
}

Comparator Megatron::ui_generate_comparator(std::string &table_name) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }

  return ui_generate_comparator(table_metadata);
}

Comparator Megatron::ui_generate_comparator(serial::TableMetadata &table_metadata) {
  std::println("Ingresa operaciones de comparacion a usar(una por linea):");
  std::println("(nombre_columna, operacion[<, >, <= , >=, ==], valor) o [AND, OR]");
  std::println("Escribe 'fin' cuando hayas terminado");

  std::vector<std::tuple<std::string, std::string, std::string>> comparisons;

  while (true) {
    std::string input;
    std::getline(std::cin, input);

    if (input == "fin") {
      break;
    }

    input.erase(input.begin(),
                std::find_if(input.begin(), input.end(),
                             [](int ch) { return !std::isspace(ch); }));
    input.erase(std::find_if(input.rbegin(), input.rend(),
                             [](int ch) { return !std::isspace(ch); })
                    .base(),
                input.end());

    if (input == "AND" || input == "OR") {
      comparisons.emplace_back("", input, "");
      std::println("Operador booleano agregado. Escribe otra o 'fin' para terminar.");
      continue;
    }

    // Caso parentesis, se borran
    input.erase(std::remove(input.begin(), input.end(), '('), input.end());
    input.erase(std::remove(input.begin(), input.end(), ')'), input.end());

    // Dividir la entrada en componentes
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = input.find(',');

    while (end != std::string::npos) {
      std::string part = input.substr(start, end - start);
      // Eliminar espacios en blanco alrededor
      part.erase(part.begin(),
                 std::find_if(part.begin(), part.end(),
                              [](int ch) { return !std::isspace(ch); }));
      part.erase(std::find_if(part.rbegin(), part.rend(),
                              [](int ch) { return !std::isspace(ch); })
                     .base(),
                 part.end());

      parts.push_back(part);
      start = end + 1;
      end = input.find(',', start);
    }

    // Ultimo componente
    std::string last_part = input.substr(start);
    last_part.erase(last_part.begin(),
                    std::find_if(last_part.begin(), last_part.end(),
                                 [](int ch) { return !std::isspace(ch); }));
    last_part.erase(std::find_if(last_part.rbegin(), last_part.rend(),
                                 [](int ch) { return !std::isspace(ch); })
                        .base(),
                    last_part.end());
    parts.push_back(last_part);

    // Verificar que tenemos exactamente 3 componentes
    if (parts.size() != 3) {
      std::println("Formato incorrecto,"
                   "ingresa: nombre_columna, operacion, valor");
      continue;
    }

    // Validar que la columna existe en los metadatos
    bool column_exists = false;
    for (auto &column : table_metadata.columns) {
      if (parts[0] == array_to_string_view(column.name)) {
        column_exists = true;
        break;
      }
    }

    if (!column_exists) {
      std::println("Error: La columna '{}' no existe en la tabla", parts[0]);
      continue;
    }

    // Validar que la operación es válida
    const std::vector<std::string> valid_ops =
        {"<", ">", "<=", ">=", "==", "!="};
    if (std::find(valid_ops.begin(), valid_ops.end(),
                  parts[1]) == valid_ops.end()) {
      std::println("Error: Operacion '{}' no valida."
                   "Usa uno de: <, >, <=, >=, ==, !=",
                   parts[1]);
      continue;
    }

    comparisons.emplace_back(parts[0], parts[1], parts[2]);
    std::println("Comparacion agregada. Escribe otra o 'fin' para terminar.");
  }

  return generate_comparator(table_metadata, comparisons);
}
