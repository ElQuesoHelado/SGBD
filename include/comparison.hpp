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
  bool is_equals{};
};

// Concatenacion de operaciones,
//.less_than(a).greater_than(b) -> (x<a) AND (x>b)
class Comparator {
  std::vector<Comparison> value_comp_ops{};

public:
  bool empty() {
    return value_comp_ops.empty();
  }

  bool is_only_equals() {
    return value_comp_ops.size() == 1 && value_comp_ops.front().is_equals;
  }

  size_t col_index_at_op(size_t i) {
    if (i >= value_comp_ops.size())
      return value_comp_ops.size();

    return value_comp_ops[i].col_index;
  }

  std::string compared_at_op(size_t i) {
    if (i >= value_comp_ops.size())
      return "";

    return SQL_type_to_string(value_comp_ops[i].compared);
  }

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
        .emplace_back(col_index, cmp_value, [](const SQL_type &x, const SQL_type &y) { return x == y; }, true, true);
    return *this;
  }

  // Se usa row para evaluar toda condicion
  bool evaluate(const std::vector<SQL_type> &row_values) {
    if (value_comp_ops.empty())
      return true;

    bool result = value_comp_ops[0].condition(
             row_values[value_comp_ops[0].col_index],
             value_comp_ops[0].compared),
         is_prev_and{value_comp_ops[0].is_next_and};

    for (size_t i = 1; i < value_comp_ops.size(); ++i) {
      const auto &op = value_comp_ops[i];
      bool partial = op.condition(row_values[op.col_index], op.compared);

      if (is_prev_and) {
        result &= partial;
      } else {
        result |= partial;
      }

      is_prev_and = op.is_next_and;
    }

    return result;
  }
};
