#include "megatron.hpp"
#include <iostream>
#include <print>

void Megatron::ui_show_threads_from_board() {
  std::string board = "board", thread = "thread";

  serial::TableMetadata board_table, thread_table;
  search_table(board, board_table);
  search_table(thread, thread_table);

  std::print("Ingresa id de board: ");
  int id_board;
  std::cin >> id_board;

  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      comparisons1 = {{"id", "==", std::to_string(id_board)}},
      comparisons2 = {{"id_board", "==", std::to_string(id_board)}};

  Comparator board_comp, thread_comp;

  board_comp = generate_comparator(board_table, comparisons1);
  thread_comp = generate_comparator(thread_table, comparisons2);

  auto boards = select(board_table, board_comp);
  auto threads = select(thread_table, board_comp);

  std::println("{} ", boards.columns);
  size_t i{1};
  for (auto &reg : boards) {
    std::println("Bloque: {} ({}) {}", reg.reg_ptr.page_id, i, reg);
    i++;
  }

  std::println("{} ", threads.columns);
  i = 1;
  for (auto &reg : threads) {
    std::println("Bloque: {} ({}) {}", reg.reg_ptr.page_id, i, reg);
    i++;
  }
}

void Megatron::ui_show_posts_from_thread() {
  std::string post = "post", thread = "thread";

  serial::TableMetadata post_table, thread_table;
  search_table(post, post_table);
  search_table(thread, thread_table);

  std::print("Ingresa id de thread: ");
  int id_thread;
  std::cin >> id_thread;

  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      comparisons1 = {{"id", "==", std::to_string(id_thread)}},
      comparisons2 = {{"id_thread", "==", std::to_string(id_thread)}};

  Comparator post_comp, thread_comp;

  thread_comp = generate_comparator(thread_table, comparisons1);
  post_comp = generate_comparator(post_table, comparisons2);

  auto threads = select(thread_table, thread_comp);
  auto posts = select(post_table, post_comp);

  std::println("{} ", threads.columns);
  size_t i{1};
  for (auto &reg : threads) {
    std::println("Bloque: {} ({}) {}", reg.reg_ptr.page_id, i, reg);
    i++;
  }

  i = 1;
  std::println("{} ", posts.columns);
  for (auto &reg : posts) {
    std::println("Bloque: {} ({}) {}", reg.reg_ptr.page_id, i, reg);
    i++;
  }
}

void Megatron::ui_delete_thread() {
}

void Megatron::ui_show_post_media() {
  std::string post = "post";

  serial::TableMetadata post_table;
  search_table(post, post_table);

  std::print("Ingresa id de post: ");
  int id_post;
  std::cin >> id_post;

  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      comparisons1 = {{"id", "==", std::to_string(id_post)}};

  Comparator post_comp;

  post_comp = generate_comparator(post_table, comparisons1);

  auto posts = select(post_table, post_comp);

  size_t i = 1;
  std::println("{} ", posts.columns);
  for (auto &reg : posts) {
    std::println("Bloque: {} ({}) {}", reg.reg_ptr.page_id, i, reg);
    i++;
  }

  std::string path = SQL_type_to_string(posts.registers.front().values[3]);
  if (path != "") {
    std::string command = "vlc " + path;
    system(command.c_str());
  }
}
