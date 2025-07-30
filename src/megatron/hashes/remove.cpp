#include "hash/hasher.hpp"

void Hasher::remove_from_set(const ResultSet &set, size_t key_col_index) {
  for (auto &r : set) {
    remove(r.values[key_col_index]);
  }
}

void Hasher::remove(const SQL_type_ &key) {
  auto bucket_id = locate_bucket(key);

  if (bucket_id == null_page_id)
    return;

  return remove_in_bucket(bucket_id, key);
}

void Hasher::remove_in_bucket(uint32_t bucket_id, const SQL_type_ &key) {
  while (bucket_id != null_page_id) {
    auto bucket = load_bucket(bucket_id);
    for (size_t j{}; j < bucket.capacity; ++j) {
      if (bucket.reg_ptrs[j].page_id != null_page_id &&
          key == bucket.keys[j]) {
        bucket.reg_ptrs[j] = {null_page_id, 0};
        bucket.size--;
      }
    }
    bucket_id = bucket.overflow_page;
    unload_bucket(bucket);
  }
}
