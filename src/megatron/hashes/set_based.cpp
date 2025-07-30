#include "megatron.hpp"

void Megatron::insert_set_hash(
    serial::TableMetadata &table_metadata, ResultSet &set) {
  auto hashed_cols = get_hashed_columns(table_metadata);
  for (auto [hashed_col, dir_id] : hashed_cols) {
    Hasher hasher(*buffer_manager,
                  dir_id,
                  table_metadata.columns[hashed_col].type,
                  table_metadata.columns[hashed_col].max_size);
    hasher.insert_from_set(set, hashed_col);
  }
}

void Megatron::delete_set_hash(
    serial::TableMetadata &table_metadata, ResultSet &set) {
  auto hashed_cols = get_hashed_columns(table_metadata);
  for (auto [hashed_col, dir_id] : hashed_cols) {
    Hasher hasher(*buffer_manager,
                  dir_id,
                  table_metadata.columns[hashed_col].type,
                  table_metadata.columns[hashed_col].max_size);
    hasher.remove_from_set(set, hashed_col);
  }
}

void Megatron::update_set_hash(
    serial::TableMetadata &table_metadata,
    ResultSet &old_set, ResultSet &new_set) {
  auto hashed_cols = get_hashed_columns(table_metadata);
  for (auto [hashed_col, dir_id] : hashed_cols) {
    Hasher hasher(*buffer_manager,
                  dir_id,
                  table_metadata.columns[hashed_col].type,
                  table_metadata.columns[hashed_col].max_size);
    hasher.update_from_set(old_set, new_set, hashed_col);
  }
}
