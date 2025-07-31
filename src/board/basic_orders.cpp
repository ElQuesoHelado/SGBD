#include "board.hpp"

ResultSet Board::get_all_threads_ordered_by_date(bool asc) {
  auto entire_range_comp = get_infinity_date_comparator(thread);

  return bd.select(thread, entire_range_comp);
}

ResultSet Board::get_all_posts_ordered_by_date(bool asc) {
  auto entire_range_comp = get_infinity_date_comparator(post);

  return bd.select(post, entire_range_comp);
}
