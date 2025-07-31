#pragma once

#include "comparison.hpp"
#include "megatron.hpp"
#include "result_set.hpp"
#include "serial/table.hpp"
#include <cstddef>

struct Board {
  std::string board_str{"board"},
      thread_str{"thread"},
      post_str{"post"};

  serial::TableMetadata board, thread, post;
  Comparator empty_comparator;

  Megatron &bd;

  Board(Megatron &bd) : bd(bd) {
    bd.search_table(board_str, board);
    bd.search_table(thread_str, thread);
    bd.search_table(post_str, post);
  }

  void translate() { bd.translate(); }

  // Genericas
  ResultSet get_all_boards();
  ResultSet get_all_threads();
  ResultSet get_all_posts();

  ResultSet get_board_on_id(size_t id);
  ResultSet get_thread_on_id(size_t id);
  ResultSet get_post_on_id(size_t id);

  ResultSet get_threads_from_board(size_t board_id);
  ResultSet get_posts_from_board(size_t board_id);
  ResultSet get_posts_from_thread(size_t thread_id);

  // Ordenamiento basico
  ResultSet get_all_threads_ordered_by_date(bool asc = true);
  ResultSet get_all_posts_ordered_by_date(bool asc = true);

  // MISCELANEA
  Comparator get_id_comparator(serial::TableMetadata &table, size_t id);
  Comparator get_board_id_comparator(size_t id);
  Comparator get_thread_id_comparator(size_t id);
  Comparator get_infinity_date_comparator(serial::TableMetadata &table);
};
