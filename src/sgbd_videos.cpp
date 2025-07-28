#include "comparison.hpp"
#include "megatron.hpp"
#include "serial/table.hpp"
#include "types/types.hpp"
#include <filesystem>
#include <print>
#include <string>
#include <utility>
#include <vector>
// #include <vlc/vlc.h>

int main(int argc, char *argv[]) {
  Megatron megatron;
  megatron.new_disk("disco_board", 20, 20,
                    20, 1000, 4,
                    20, 0);

  std::vector<std::pair<std::string, std::string>>
      board_cols = {
          {"id", "SMALLINT"},
          {"codigo", "CHAR(10)"},
          {"nombre", "CHAR(20)"},
          {"descripcion", "CHAR(50)"},
      },
      thread_cols = {
          {"id", "SMALLINT"},
          {"id_board", "SMALLINT"},
          {"nombre", "CHAR(50)"},
          {"puntaje", "SMALLINT"},
      },
      post_cols = {
          {"id", "SMALLINT"},
          {"id_thread", "SMALLINT"},
          {"contenido", "CHAR(50)"},
          {"path", "CHAR(20)"},
      };

  std::string board = "board", thread = "thread", post = "post";

  megatron.create_table(board, board_cols);
  megatron.create_table(thread, thread_cols);
  megatron.create_table(post, post_cols);

  megatron.load_CSV("csv/board.csv",
                    board);

  megatron.load_CSV("csv/thread.csv",
                    thread);

  megatron.load_CSV("csv/post.csv",
                    post);

  // megatron.add_hash_to_table(std::string &table_name, std::string &col_name)
  megatron.add_hash_to_table(board, board_cols[0].first);
  megatron.add_hash_to_table(thread, thread_cols[0].first);
  megatron.add_hash_to_table(post, post_cols[0].first);

  megatron.add_index_to_table(thread, thread_cols[3].first);
  megatron.add_index_to_table(post, post_cols[1].first);

  megatron.run();
  // Comparator comp;

  // megatron.select_print(board, comp);
  // megatron.select_print(thread, comp);
  // megatron.select_print(post, comp);
  // megatron.ui_show_posts_from_thread();
  // megatron.ui_show_post_media();
  // megatron.ui_show_posts_from_thread();
}
