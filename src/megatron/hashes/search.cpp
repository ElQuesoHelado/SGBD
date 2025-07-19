#include "hash/hasher.hpp"
#include "types.hpp"
#include <cstddef>

std::vector<RegPtr> Hasher::search(Comparator &equals) {
  auto key = equals.compared_at_op(0);

  auto bucket_id = locate_bucket(key);

  if (bucket_id == null_page_id)
    return {};

  return search_in_bucket(bucket_id, key);
}

std::vector<RegPtr> Hasher::search_in_bucket(uint32_t bucket_id,
                                             SQL_type &key) {
  std::vector<RegPtr> reg_ptrs{};

  while (bucket_id != null_page_id) {
    auto bucket = load_bucket(bucket_id);
    for (size_t i{}, j{}; i < bucket.size && j < bucket.capacity; ++j) {
      if (bucket.reg_ptrs[j].page_id != null_page_id &&
          key == bucket.keys[j]) {
        reg_ptrs.push_back(bucket.reg_ptrs[j]);
        ++i;
      }
    }
    bucket_id = bucket.overflow_page;
    unload_bucket(bucket);
  }

  return reg_ptrs;
}
