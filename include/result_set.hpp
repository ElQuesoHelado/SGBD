#pragma once

#include "serial/generic.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <cstddef>
#include <format>
#include <iostream>
#include <string>
#include <vector>

struct RegisterEntry {
  size_t page_id{};
  size_t position{};
  std::vector<SQL_type> values{};
};

// Solo valores
inline std::ostream &operator<<(std::ostream &os, const RegisterEntry &entry) {
  for (const auto &value : entry.values)
    os << SQL_type_to_string(value) << " ";

  return os;
}

template <>
struct std::formatter<RegisterEntry> : std::formatter<std::string> {
  auto format(const RegisterEntry &entry, std::format_context &ctx) const {
    std::string values_str;
    for (auto &v : entry.values)
      values_str += SQL_type_to_string(v) + " ";

    return std::formatter<std::string>::format(std::format("{}", values_str), ctx);
  }
};

// Clase que representa resultados de cierta operacion
// @note Registros en formato texto con paginas y posicion donde se opero
class ResultSet {

public:
  std::vector<std::string> columns{};

  std::vector<RegisterEntry> registers{};

  // std::vector<std::string>types ; ?tipos de cada columna?

  auto begin() const { return registers.begin(); }
  auto end() const { return registers.end(); }

  void add_columns(std::vector<serial::Column> &raw_columns) {
    for (auto &c : raw_columns)
      columns.emplace_back(array_to_string_view(c.name));
  }

  void add_register(RegisterEntry &&reg) {
    registers.push_back(reg);
  }

  // Agrega todos los registros de otro set
  void merge(ResultSet &&other) {
    registers.reserve(registers.size() + other.registers.size());
    registers.insert(registers.end(),
                     std::make_move_iterator(other.registers.begin()),
                     std::make_move_iterator(other.registers.end()));
  }

  size_t size() {
    return registers.size();
  }

  bool empty() {
    return registers.empty();
  }
};
