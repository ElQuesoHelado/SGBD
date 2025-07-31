#include "board.hpp"
#include "result_set.hpp"

ResultSet Board::delete_posts_in_date_range(size_t low_date, size_t high_date) {
  auto date_comp =
      get_ranged_date_comp(post, low_date, high_date);

  auto set =
      bd.delete_condition(post, date_comp);

  return set;
}

ResultSet Board::delete_thread(size_t thread_id) {
  auto thread_id_comp = get_thread_id_comparator(thread_id);
  auto id_comp = get_id_comparator(thread, thread_id);

  auto set =
      bd.delete_condition(post, thread_id_comp);

  bd.delete_condition(thread, id_comp);

  return set;
}
