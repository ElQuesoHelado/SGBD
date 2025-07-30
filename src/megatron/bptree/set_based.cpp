#include "megatron.hpp"

void Megatron::insert_set_index(
    serial::TableMetadata &table_metadata, ResultSet &set) {
  auto indexed_cols = get_indexed_columns(table_metadata);
  for (auto [indexed_col, root_id] : indexed_cols) {
    size_t min_degree =
        calculate_btree_order(table_metadata.columns[indexed_col].max_size);
    BPTree tree(*buffer_manager, table_metadata,
                root_id,
                min_degree,
                table_metadata.columns[indexed_col].type,
                table_metadata.columns[indexed_col].max_size);

    tree.insert_from_set(set, indexed_col);
  }
}

void Megatron::delete_set_index(
    serial::TableMetadata &table_metadata, ResultSet &set) {
  auto indexed_cols = get_indexed_columns(table_metadata);
  for (auto [indexed_col, root_id] : indexed_cols) {
    size_t min_degree =
        calculate_btree_order(table_metadata.columns[indexed_col].max_size);
    BPTree tree(*buffer_manager, table_metadata,
                root_id,
                min_degree,
                table_metadata.columns[indexed_col].type,
                table_metadata.columns[indexed_col].max_size);

    tree.remove_from_set(set, indexed_col);
  }
}

void Megatron::update_set_index(
    serial::TableMetadata &table_metadata,
    ResultSet &old_set, ResultSet &new_set) {
  auto indexed_cols = get_indexed_columns(table_metadata);
  for (auto [indexed_col, root_id] : indexed_cols) {
    size_t min_degree =
        calculate_btree_order(table_metadata.columns[indexed_col].max_size);
    BPTree tree(*buffer_manager, table_metadata,
                root_id,
                min_degree,
                table_metadata.columns[indexed_col].type,
                table_metadata.columns[indexed_col].max_size);

    tree.update_from_set(old_set, new_set, indexed_col);
  }
}
