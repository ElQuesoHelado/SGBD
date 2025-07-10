#include "hlpr.hpp"
#include "megatron.hpp"

ResultSet Megatron::delete_hashed(serial::TableMetadata table_metadata,
                                  size_t hashed_col, SQL_type &key) {
  if (!is_column_hashed(table_metadata, hashed_col)) {
    std::cerr << "No se tiene hash para esa columna" << std::endl;
    return {};
  }

  auto dir_page = get_root_page_id(table_metadata, hashed_col);

  DirectoryPage dir = load_directory_page(dir_page);

  std::vector<unsigned char> key_bytes = serialize_sql_type(key);

  // En base a hash se ubica Bucket
  uint32_t bucket_ptr_index = hash_index(key_bytes,
                                         dir.global_depth);

  BucketPtr &bucket_ptr = dir.bucket_ptrs[bucket_ptr_index];

  ResultSet result_set{};
  result_set.add_columns(table_metadata.columns);

  size_t mb_page_id = bucket_ptr.mb_page;
  while (true) {
    auto &frame = buffer_manager->load_pin_page(mb_page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;

    MultiBucketPage mb_page = deserialize_multi_bucket_page(page_bytes);
    Bucket_ &bucket = mb_page.buckets[bucket_ptr.bucket_idx];

    // Se revisa todo registro en bucket(caso keys duplicadas)
    for (uint8_t i = 0; i < bucket.reg_ptr_count; i++) {
      auto partial_result_set =
          get_register_on_key_match(table_metadata,
                                    bucket.reg_ptrs[i],
                                    hashed_col,
                                    key);

      result_set.merge(std::move(partial_result_set));

      // Se hizo match, lo eliminamos
      if (!partial_result_set.empty()) {
        bucket.reg_ptrs[i].page_id = disk_manager->NULL_BLOCK;
        bucket.reg_ptrs[i].hash = 0;
        bucket.reg_ptr_count--;
      }
    }

    // Se continua busqueda en overflow page
    if (bucket.overflow_page == disk_manager->NULL_BLOCK)
      break;

    buffer_manager->free_unpin_page(bucket_ptr.mb_page, 1);

    mb_page_id = bucket.overflow_page;
  }

  buffer_manager->free_unpin_page(bucket_ptr.mb_page, 1);
  return result_set;
}
