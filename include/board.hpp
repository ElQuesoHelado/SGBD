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

  void load_posts(size_t n);

  // Ordenamiento basico
  ResultSet get_all_threads_ordered_by_date(bool asc = true);
  ResultSet get_all_posts_ordered_by_date(bool asc = true);
  ResultSet get_threads_from_board_ordered_by_date(size_t board_id, bool asc = true);
  ResultSet get_posts_from_board_ordered_by_date(size_t board_id, bool asc = true);
  ResultSet get_posts_from_thread_ordered_by_date(size_t thread_id, bool asc = true);

  ResultSet get_all_posts_in_date_range(size_t low_date,
                                        size_t high_date, bool asc = true);
  ResultSet get_posts_in_date_range_from_thread(size_t thread_id,
                                                size_t low_date,
                                                size_t high_date, bool asc = true);
  ResultSet get_posts_in_date_range_from_board(size_t board_id, size_t low_date,
                                               size_t high_date, bool asc = true);

  ResultSet delete_posts_in_date_range(size_t low_date, size_t high_date);

  ResultSet delete_thread(size_t thread_id);

  // MISCELANEA/HELPERS
  Comparator get_id_comparator(serial::TableMetadata &table, size_t id);
  Comparator get_board_id_comparator(size_t board_id);
  Comparator get_thread_id_comparator(size_t thread_id);
  Comparator get_inf_date_comp(serial::TableMetadata &table);
  Comparator get_ranged_date_comp(serial::TableMetadata &table,
                                  size_t low_date, size_t high_date);
};
