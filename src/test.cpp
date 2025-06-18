#include "disk_manager.hpp"
#include "megatron.hpp"
#include "serial/generic.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>

#include "types.hpp"
#include <string_view>
#include <type_traits>
#include <vector>

std::vector<std::pair<std::string, std::string>> determine_column_types(
    const std::vector<std::string> &headers,
    const std::vector<std::vector<std::string>> &rows,
    bool prefer_varchar) {

  if (headers.empty() || rows.empty())
    return {};

  const size_t num_cols = headers.size();
  std::vector<std::string> current_types(num_cols, "UNKNOWN");
  std::vector<size_t> max_lengths(num_cols, 0);
  std::vector<bool> type_locked(num_cols, false);

  auto promote_type = [](const std::string &current, const std::string &new_type) {
    const static std::vector<std::string> type_hierarchy = {
        "UNKNOWN", "TINYINT", "SMALLINT", "INTEGER", "BIGINT", "FLOAT", "DOUBLE", "CHAR", "VARCHAR"};

    auto get_rank = [](const std::string &type) {
      if (type.find("VARCHAR") != std::string::npos)
        return 8;
      if (type.find("CHAR") != std::string::npos)
        return 7;
      for (size_t i = 0; i < type_hierarchy.size(); ++i) {
        if (type == type_hierarchy[i])
          return (int)i;
      }
      return 0;
    };

    size_t current_rank = get_rank(current);
    size_t new_rank = get_rank(new_type);
    return current_rank > new_rank ? current : new_type;
  };

  for (const auto &row : rows) {
    for (size_t col = 0; col < num_cols && col < row.size(); ++col) {
      const std::string &value = row[col];
      max_lengths[col] = std::max(max_lengths[col], value.size());

      if (value.empty())
        continue; // Skip empty values but keep tracking max_length

      std::string detected_type = "UNKNOWN";

      // Try to detect type
      try {
        size_t pos = 0;
        int64_t int_val = std::stoll(value, &pos);

        if (pos == value.size()) { // Entire string is a number
          if (int_val >= INT8_MIN && int_val <= INT8_MAX) {
            detected_type = "TINYINT";
          } else if (int_val >= INT16_MIN && int_val <= INT16_MAX) {
            detected_type = "SMALLINT";
          } else if (int_val >= INT32_MIN && int_val <= INT32_MAX) {
            detected_type = "INTEGER";
          } else {
            detected_type = "BIGINT";
          }
        }
      } catch (...) {
        // Not an integer
      }

      if (detected_type == "UNKNOWN") {
        try {
          size_t pos = 0;
          double float_val = std::stod(value, &pos);

          if (pos == value.size()) { // Entire string is a float
            if (float_val >= std::numeric_limits<float>::lowest() && float_val <= std::numeric_limits<float>::max() &&
                static_cast<float>(float_val) == float_val) {
              detected_type = "FLOAT";
            } else {
              detected_type = "DOUBLE";
            }
          }
        } catch (...) {
          // Not a number, treat as string
          detected_type = prefer_varchar ? "VARCHAR" : "CHAR";
        }
      }

      if (!type_locked[col]) {
        if (current_types[col] == "UNKNOWN") {
          current_types[col] = detected_type;
        } else {
          current_types[col] = promote_type(current_types[col], detected_type);
        }

        // Once a column is determined to be string, it can't go back to numeric
        if (current_types[col].find("CHAR") != std::string::npos) {
          type_locked[col] = true;
        }
      }
    }
  }

  // Final processing
  std::vector<std::pair<std::string, std::string>> result;
  for (size_t col = 0; col < num_cols; ++col) {
    std::string final_type = current_types[col];

    // Handle columns that were all empty
    if (final_type == "UNKNOWN") {
      // Check neighboring columns for type hints
      bool found_numeric = false;
      for (size_t i = 0; i < num_cols; ++i) {
        if (i != col && current_types[i] != "UNKNOWN" &&
            current_types[i].find("CHAR") == std::string::npos) {
          found_numeric = true;
          break;
        }
      }
      final_type = found_numeric ? "TINYINT" : (prefer_varchar ? "VARCHAR(1)" : "CHAR(1)");
    }

    // Add length for string types
    if (final_type == "CHAR" || final_type == "VARCHAR") {
      final_type += "(" + std::to_string(std::max<size_t>(1, max_lengths[col])) + ")";
    }

    result.emplace_back(headers[col], final_type);
  }

  return result;
}

void load_CSV_with_schema_inference(std::string path, std::string table_name, bool prefer_varchar) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Error: No se pudo abrir el archivo " << path << std::endl;
    return;
  }

  // Leer todas las filas del CSV
  std::vector<std::string> headers;
  std::vector<std::vector<std::string>> rows;
  std::string line;

  // Leer cabeceras
  if (std::getline(file, line)) {
    std::istringstream header_ss(line);
    std::string header;
    while (std::getline(header_ss, header, ',')) {
      headers.push_back(header);
    }
  }

  // Leer filas de datos
  while (std::getline(file, line)) {
    std::istringstream line_ss(line);
    std::string value;
    std::vector<std::string> row;

    while (std::getline(line_ss, value, ',')) {
      // Eliminar comillas si existen
      if (!value.empty() && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
      }
      row.push_back(value);
    }

    // Asegurar que todas las filas tengan el mismo número de columnas
    if (row.size() < headers.size()) {
      row.resize(headers.size());
    } else if (row.size() > headers.size()) {
      row.resize(headers.size());
    }

    rows.push_back(row);
  }

  // Determinar tipos de columna
  auto columns = determine_column_types(headers, rows, prefer_varchar);

  // // Verificar si la tabla ya existe
  serial::TableMetadata existing_metadata;
  // if (search_table(table_name, existing_metadata)) {
  //   std::cerr << "Error: La tabla " << table_name << " ya existe" << std::endl;
  //   return;
  // }
  //
  // // Crear la tabla
  // serial::TableMetadata new_metadata;
  // uint32_t block_id = 0; // O obtener el próximo block_id disponible según tu sistema
  //
  // try {
  //   init_table_metadata(new_metadata, table_name, block_id, columns);
  //   save_table_metadata(new_metadata);
  //   std::cout << "Tabla " << table_name << " creada exitosamente" << std::endl;
  // } catch (const std::exception &e) {
  //   std::cerr << "Error al crear la tabla: " << e.what() << std::endl;
  //   return;
  // }
  //
  // // Insertar los datos
  // try {
  //   for (const auto &row : rows) {
  //     if (new_metadata.are_regs_fixed) {
  //       insert_fixed(new_metadata, row);
  //     } else {
  //       insert_slotted(new_metadata, row);
  //     }
  //   }
  //   std::cout << rows.size() << " registros insertados exitosamente" << std::endl;
  // } catch (const std::exception &e) {
  //   std::cerr << "Error al insertar datos: " << e.what() << std::endl;
  // }
}

int main(int argc, char *argv[]) {
  load_CSV_with_schema_inference("csv/housing.csv", "ejemplo", 0);
}
