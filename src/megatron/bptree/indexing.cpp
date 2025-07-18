#include "bptree/bptree.hpp"
#include "megatron.hpp"
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>

void Megatron::reindex_table(serial::TableMetadata &table_metadata) {
  auto indexed_cols = get_indexed_columns(table_metadata);

  Comparator comp;
  auto registers = select(table_metadata, comp);

  // Para cada indice se va a realizar un insert
  for (auto ic : indexed_cols) {
    auto col_index = ic.first;
    auto root_id = ic.second;
    // auto min_degree =
    //     calculate_btree_order(table_metadata.columns[col_index].max_size);
    auto min_degree = 2;

    BPTree tree(*buffer_manager, table_metadata,
                disk_manager->NULL_BLOCK, root_id,
                min_degree,
                table_metadata.columns[col_index].type,
                table_metadata.columns[col_index].max_size);

    for (auto &reg : registers) {
      tree.insert(reg.values[col_index], {reg.page_id, reg.position});
    }
  }
}
