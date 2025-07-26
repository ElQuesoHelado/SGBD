#include "hash/hasher.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>

void Hasher::insert_from_set(const ResultSet &set, size_t key_col_index) {
  for (auto &r : set) {
    insert(r.values[key_col_index], {r.page_id, r.position});
  }
}

void Hasher::insert(const SQL_type_ &key, const RegPtr &reg_ptr) {
  auto bucket_id = locate_bucket(key);

  auto bucket = load_bucket(bucket_id);

  if (bucket.size < bucket.capacity) {
    insert_non_full_bucket(bucket, key, reg_ptr);
    unload_bucket(bucket, true);
  } else {
    // Revisamos colisiones no resolvibles por split
    auto k = hash_key(key, bucket.local_depth + 1);
    if (check_bucket_keys_same_prefix(bucket, bucket.local_depth + 1) && true) {
      // Se busca un overflow bucket
      unload_bucket(bucket);
      insert_overflow_bucket(bucket.page_id, key, reg_ptr);
    } else {
      unload_bucket(bucket);
      split_bucket(bucket.page_id);
    }
  }
}

void Hasher::insert_non_full_bucket(
    Bucket &bucket, const SQL_type_ &key, const RegPtr &reg_ptr) {
  for (size_t j{}; j < bucket.capacity; ++j) {
    if (bucket.reg_ptrs[j].page_id == null_page_id) {
      bucket.keys[j] = key;
      bucket.reg_ptrs[j] = reg_ptr;
      bucket.size++;
      return;
    }
  }
}

void Hasher::insert_overflow_bucket(
    uint32_t bucket_id, const SQL_type_ &key, const RegPtr &reg_ptr) {
  std::println("overflow bucket creado");
  auto bucket = load_bucket(bucket_id);

  if (bucket.size == bucket.capacity) {
    if (bucket.overflow_page == null_page_id) {
      auto new_bucket =
          allocate_bucket(bucket.prefix,
                          bucket.local_depth);
      bucket.overflow_page = new_bucket.page_id;
      unload_bucket(new_bucket, true);
    }

    unload_bucket(bucket);
    insert_overflow_bucket(bucket.overflow_page, key, reg_ptr);
  } else {
    insert_non_full_bucket(bucket, key, reg_ptr);
  }
  unload_bucket(bucket, true);
}
