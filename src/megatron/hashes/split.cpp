#include "hash/hasher.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>

void Hasher::split_bucket(uint32_t bucket_id) {
  auto directory = load_directory(directory_id);

  auto old_bucket = load_bucket(bucket_id);
  auto new_bucket =
      allocate_bucket(
          (1 << old_bucket.local_depth) | old_bucket.prefix,
          old_bucket.local_depth + 1);

  old_bucket.local_depth++;

  for (size_t i{}; i < old_bucket.capacity; ++i) {
    if (old_bucket.reg_ptrs[i].page_id != null_page_id &&
        old_bucket.prefix !=
            hash_key(old_bucket.keys[i], old_bucket.local_depth)) {
      insert_non_full_bucket(new_bucket,
                             old_bucket.keys[i],
                             old_bucket.reg_ptrs[i]);

      old_bucket.reg_ptrs[i] = {null_page_id, 0};
      old_bucket.size--;
    }
  }

  directory.bucket_ptrs[new_bucket.prefix] = new_bucket.page_id;

  unload_bucket(old_bucket, true);
  unload_bucket(new_bucket, true);
  unload_directory(directory);
}
