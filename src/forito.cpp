#include "megatron.hpp"

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

  megatron.add_hash_to_table(board, board_cols[0].first);
  megatron.add_hash_to_table(thread, thread_cols[0].first);
  megatron.add_hash_to_table(post, post_cols[0].first);

  megatron.add_index_to_table(thread, thread_cols[3].first);
  megatron.add_index_to_table(post, post_cols[3].first);
  //
  // megatron.translate();

  megatron.run();
  // Comparator comp;

  // megatron.select_print(board, comp);
  // megatron.select_print(thread, comp);
  // megatron.select_print(post, comp);
  // megatron.ui_show_posts_from_thread();
  // megatron.ui_show_post_media();
  // megatron.ui_show_posts_from_thread();
}
