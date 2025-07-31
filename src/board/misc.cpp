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

Comparator Board::get_board_id_comparator(size_t id) {
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      x = {{"id_board", "==", std::to_string(id)}};

  return bd.generate_comparator(thread, x);
}

Comparator Board::get_thread_id_comparator(size_t id) {
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      x = {{"id_thread", "==", std::to_string(id)}};

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
