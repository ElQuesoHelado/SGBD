#include "board.hpp"
#include "result_set.hpp"

ResultSet Board::get_all_boards() {
  return bd.select(board, empty_comparator);
}

ResultSet Board::get_all_threads() {
  return bd.select(thread, empty_comparator);
}

ResultSet Board::get_all_posts() {
  return bd.select(post, empty_comparator);
}

ResultSet Board::get_board_on_id(size_t id) {
  auto comp = get_id_comparator(board, id);

  return bd.select(board, comp);
}

ResultSet Board::get_thread_on_id(size_t id) {
  auto comp = get_id_comparator(thread, id);

  return bd.select(thread, comp);
}
ResultSet Board::get_post_on_id(size_t id) {
  auto comp = get_id_comparator(post, id);

  return bd.select(post, comp);
}

ResultSet Board::get_threads_from_board(size_t board_id) {
  auto board_id_comp = get_board_id_comparator(board_id);
  return bd.select(thread, board_id_comp);
}

ResultSet Board::get_posts_from_board(size_t board_id) {
  auto board_id_comp = get_board_id_comparator(board_id);
  auto threads_result =
      bd.select(thread, board_id_comp);

  ResultSet posts_result;
  for (auto &t : threads_result) {

    auto thread_id_comp =
        get_thread_id_comparator(std::get<int64_t>(t.values[0]));

    posts_result.merge(get_posts_from_thread(
        std::get<int64_t>(t.values[0])));
  }

  return posts_result;
}

ResultSet Board::get_posts_from_thread(size_t thread_id) {
  auto thread_id_comp =
      get_thread_id_comparator(thread_id);

  return bd.select(post, thread_id_comp);
}
