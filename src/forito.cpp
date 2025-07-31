#include "board.hpp"
#include "httplib.h"
#include "megatron.hpp"
#include <print>

int main(int argc, char *argv[]) {
  Megatron megatron;
  megatron.new_disk("disco_foro", 20, 20,
                    20, 1000, 4,
                    100, 0);

  std::vector<std::pair<std::string, std::string>>
      board_cols = {
          {"id", "SMALLINT"},
          {"codigo", "VARCHAR(10)"},
          {"nombre", "VARCHAR(20)"},
          {"descripcion", "VARCHAR(250)"},
      },
      thread_cols = {
          {"id", "BIGINT"},
          {"id_board", "SMALLINT"},
          {"nombre", "VARCHAR(250)"},
          {"fecha", "BIGINT"},
      },
      post_cols = {
          {"id", "BIGINT"},
          {"id_thread", "BIGINT"},
          {"contenido", "VARCHAR(250)"},
          {"fecha", "BIGINT"},
          {"path", "VARCHAR(250)"},
      };

  std::string board_str = "board", thread_str = "thread", post_str = "post";

  megatron.create_table(board_str, board_cols);
  megatron.create_table(thread_str, thread_cols);
  megatron.create_table(post_str, post_cols);

  megatron.load_CSV("csv/board.csv",
                    board_str);

  megatron.load_CSV("csv/thread.csv",
                    thread_str);

  megatron.load_CSV("csv/post.csv",
                    post_str, 500);

  megatron.add_hash_to_table(board_str, board_cols[0].first);
  megatron.add_hash_to_table(thread_str, thread_cols[0].first);
  megatron.add_hash_to_table(post_str, post_cols[0].first);

  megatron.add_index_to_table(thread_str, thread_cols[3].first);
  megatron.add_index_to_table(post_str, post_cols[3].first);

  Board board(megatron);

  std::println("{}", board.get_all_threads_ordered_by_date());
  // std::println("{}", board.get_all_threads());

  megatron.translate();

  // httplib::Server svr;
  //
  // svr.Get("/a", [&megatron](const httplib::Request &req, httplib::Response &res) {
  //   std::string table_name = "board";
  //   Comparator comp;
  //   auto results = megatron.select(table_name, comp);
  //   std::string html = results.columns.back();
  //   res.set_content(html, "text/html");
  // });
  //
  // svr.Get("/shutdown", [&](const httplib::Request &, httplib::Response &res) {
  //   svr.stop();
  //   return;
  // });
  //
  // svr.listen("localhost", 8080);

  // megatron.run();
  //  Comparator comp;

  // megatron.select_print(board, comp);
  // megatron.select_print(thread, comp);
  // megatron.select_print(post, comp);
  // megatron.ui_show_posts_from_thread();
  // megatron.ui_show_post_media();
  // megatron.ui_show_posts_from_thread();
}
