#pragma once

#include "serial/generic.hpp"
#include "serial/table.hpp"
#include <cstddef>
#include <string>
#include <vector>

// Clase que representa resultados de cierta operacion
// @note Registros en formato texto con paginas y posicion donde se opero
class ResultSet {
  struct RegisterEntry {
    size_t page_id{};
    size_t position{};
    std::vector<std::string> values{};
  };

public:
  std::vector<std::string> columns{};
  // std::vector<std::string>types ;

  auto begin() const { return registers.begin(); }
  auto end() const { return registers.end(); }

  std::vector<RegisterEntry> registers{};

  void add_columns(std::vector<serial::Column> &raw_columns) {
    for (auto &c : raw_columns)
      columns.emplace_back(array_to_string_view(c.name));
  }

  void add_register(RegisterEntry &&reg) {
    registers.push_back(reg);
  }

  size_t size() {
    return registers.size();
  }
};
