#include "board.hpp"
#include "comparison.hpp"
#include "serial/table.hpp"
#include <cstddef>
#include <string>

Comparator Board::get_id_comparator(serial::TableMetadata &table, size_t id) {
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      x = {{"id", "==", std::to_string(id)}};

  return bd.generate_comparator(table, x);
}

Comparator Board::get_board_id_comparator(size_t board_id) {
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      x = {{"id_board", "==", std::to_string(board_id)}};

  return bd.generate_comparator(thread, x);
}

Comparator Board::get_thread_id_comparator(size_t thread_id) {
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      x = {{"id_thread", "==", std::to_string(thread_id)}};

  return bd.generate_comparator(post, x);
}

Comparator Board::get_inf_date_comp(serial::TableMetadata &table) {
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      comp_str = {
          {"fecha", ">", "0"},
          {"fecha", "<", "999999999999"}};

  return bd.generate_comparator(table, comp_str);
}

Comparator Board::get_ranged_date_comp(serial::TableMetadata &table,
                                       size_t low_date, size_t high_date) {
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      comp_str = {
          {"fecha", ">=", std::to_string(low_date)},
          {"fecha", "<=", std::to_string(high_date)}};

  return bd.generate_comparator(table, comp_str);
}
