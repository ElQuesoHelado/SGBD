#include "board.hpp"
#include <cstddef>

ResultSet Board::get_all_posts_in_date_range(
    size_t low_date,
    size_t high_date, bool asc) {
  auto date_comp =
      get_ranged_date_comp(post, low_date, high_date);

  auto set =
      bd.select(post, date_comp);

  if (!asc)
    std::reverse(set.registers.begin(), set.registers.end());

  return set;
}

ResultSet Board::get_posts_in_date_range_from_thread(
    size_t thread_id,
    size_t low_date,
    size_t high_date, bool asc) {
  auto set = get_all_posts_in_date_range(low_date, high_date);

  auto it = std::remove_if(
      set.registers.begin(),
      set.registers.end(),
      [thread_id](const RegisterEntry &entry) {
        return std::get<int16_t>(entry.values[1]) != (uint16_t)thread_id;
      });

  set.registers.erase(it, set.end());

  return set;
}

ResultSet Board::get_posts_in_date_range_from_board(
    size_t board_id, size_t low_date,
    size_t high_date, bool asc) {
  auto board_id_comp = get_board_id_comparator(board_id);
  auto threads_result =
      bd.select(thread, board_id_comp);

  ResultSet posts_result;
  for (auto &t : threads_result) {

    auto thread_id_comp =
        get_thread_id_comparator(std::get<int64_t>(t.values[0]));

    posts_result.merge(get_posts_in_date_range_from_thread(
        std::get<int64_t>(t.values[0]), low_date, high_date));
  }

  return posts_result;
}
