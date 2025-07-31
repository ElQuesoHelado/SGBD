#include "board.hpp"
#include "result_set.hpp"
#include <cstdint>

ResultSet Board::get_all_threads_ordered_by_date(bool asc) {
  auto entire_range_comp =
      get_inf_date_comp(thread);

  auto set =
      bd.select(thread, entire_range_comp);

  if (!asc)
    std::reverse(set.registers.begin(), set.registers.end());

  return set;
}

ResultSet Board::get_all_posts_ordered_by_date(bool asc) {
  auto entire_range_comp =
      get_inf_date_comp(post);

  auto set =
      bd.select(post, entire_range_comp);
  if (!asc)
    std::reverse(set.registers.begin(), set.registers.end());

  return set;
}

ResultSet Board::get_threads_from_board_ordered_by_date(size_t board_id, bool asc) {
  auto board_id_comp = get_inf_date_comp(thread);

  auto set =
      bd.select(thread, board_id_comp);
  if (!asc)
    std::reverse(set.registers.begin(), set.registers.end());

  auto it = std::remove_if(
      set.registers.begin(),
      set.registers.end(),
      [board_id](const RegisterEntry &entry) {
        return std::get<int16_t>(entry.values[1]) != (uint16_t)board_id;
      });

  set.registers.erase(it, set.end());

  return set;
}

ResultSet Board::get_posts_from_board_ordered_by_date(size_t board_id, bool asc) {
  auto board_id_comp = get_board_id_comparator(board_id);
  auto threads_result =
      bd.select(thread, board_id_comp);

  ResultSet posts_result;
  for (auto &t : threads_result) {

    auto thread_id_comp =
        get_thread_id_comparator(std::get<int64_t>(t.values[0]));

    posts_result.merge(get_posts_from_thread_ordered_by_date(
        std::get<int64_t>(t.values[0])));
  }

  return posts_result;
}

ResultSet Board::get_posts_from_thread_ordered_by_date(size_t thread_id, bool asc) {
  auto entire_range_comp = get_inf_date_comp(post);

  auto set =
      bd.select(post, entire_range_comp);
  if (!asc)
    std::reverse(set.registers.begin(), set.registers.end());

  auto it = std::remove_if(
      set.registers.begin(),
      set.registers.end(),
      [thread_id](const RegisterEntry &entry) {
        return std::get<int64_t>(entry.values[1]) != (uint64_t)thread_id;
      });

  set.registers.erase(it, set.end());

  return set;
}
