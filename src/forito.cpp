#include "board.hpp"
#include "board_ui.hpp"
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
                    post_str);

  megatron.add_hash_to_table(board_str, board_cols[0].first);
  megatron.add_hash_to_table(thread_str, thread_cols[0].first);
  megatron.add_hash_to_table(post_str, post_cols[0].first);

  megatron.add_index_to_table(thread_str, thread_cols[3].first);
  megatron.add_index_to_table(post_str, post_cols[3].first);

  // /* Caso quiera cargar disco nomas p
  // Megatron megatron;
  // megatron.load_disk("disco_foro", 100, 0);
  // */

  Board board(megatron);

  BoardInterface board_ui(board);

  board_ui.run();
}
