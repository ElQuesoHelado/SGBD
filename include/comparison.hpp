#pragma once
#include "types.hpp"

#include <cstddef>
#include <functional>
#include <vector>

struct Comparison {
  size_t col_index{};
  SQL_type compared;
  std::function<bool(const SQL_type &, const SQL_type &)> condition;
  bool is_next_and{true}; // representa (A < B) and/or ...
};

// Concatenacion de operaciones,
//.less_than(a).greater_than(b) -> (x<a) AND (x>b)
class Comparator {
  std::vector<Comparison> value_comp_ops{};

public:
  bool empty() { return value_comp_ops.empty(); }

  Comparator &AND() {
    if (!empty())
      value_comp_ops.back().is_next_and = true;
    return *this;
  }

  Comparator &OR() {
    if (!empty())
      value_comp_ops.back().is_next_and = false;
    return *this;
  }

  Comparator &less_than(const SQL_type &cmp_value, size_t col_index) {
    value_comp_ops
        .emplace_back(
            col_index,
            cmp_value,
            [](const SQL_type &x, const SQL_type &y) { return x < y; }, true);
    return *this;
  }

  Comparator &greater_than(const SQL_type &cmp_value, size_t col_index) {
    value_comp_ops
        .emplace_back(col_index, cmp_value, [](const SQL_type &x, const SQL_type &y) { return x > y; }, true);
    return *this;
  }

  Comparator &less_equal_than(const SQL_type &cmp_value, size_t col_index) {
    value_comp_ops
        .emplace_back(col_index, cmp_value, [](const SQL_type &x, const SQL_type &y) { return x <= y; }, true);
    return *this;
  }

  Comparator &greater_equal_than(const SQL_type &cmp_value, size_t col_index) {
    value_comp_ops
        .emplace_back(col_index, cmp_value, [](const SQL_type &x, const SQL_type &y) { return x >= y; }, true);
    return *this;
  }

  Comparator &equals(const SQL_type &cmp_value, size_t col_index) {
    value_comp_ops
        .emplace_back(col_index, cmp_value, [](const SQL_type &x, const SQL_type &y) { return x == y; }, true);
    return *this;
  }

  // Se usa row para evaluar toda condicion
  bool evaluate(const std::vector<SQL_type> &row_values) {
    if (value_comp_ops.empty())
      return true;

    // Caso and/or varia valor inicial
    bool res{};
    if (value_comp_ops.front().is_next_and)
      res = true;

    for (auto &c : value_comp_ops) {
      bool partial = c.condition(row_values[c.col_index], c.compared);
      if (c.is_next_and)
        res &= partial;
      else
        res |= partial;
    }

    return res;
  }
};
