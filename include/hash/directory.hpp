#pragma once

#include "serial/generic.hpp"
#include <cstdint>
#include <vector>

#pragma pack(push, 1)
// Necesario ya que un bucket
// se encuentra en un array de Buckets
struct BucketPtr {
  uint32_t mb_page; // Multi-bucket page
  uint8_t bucket_idx;
  uint8_t local_depth;
};
#pragma pack(pop)

struct DirectoryPage {
  uint32_t next_page;
  uint16_t global_depth;
  uint16_t bucket_ptr_count;
  std::vector<BucketPtr> bucket_ptrs;
};

template <typename Iter>
inline void serialize_directory_page(const DirectoryPage &dir_page, Iter &out_it) {
  write_v(out_it, dir_page.next_page);
  write_v(out_it, dir_page.global_depth);
  write_v(out_it, dir_page.bucket_ptr_count);

  for (auto &b : dir_page.bucket_ptrs)
    write_v(out_it, b);
}

inline DirectoryPage deserialize_directory_page(
    std::vector<unsigned char> &bytes) {
  DirectoryPage dir_page;
  auto it = bytes.begin();

  read_v(it, dir_page.next_page);
  read_v(it, dir_page.global_depth);
  read_v(it, dir_page.bucket_ptr_count);

  dir_page.bucket_ptrs.resize(dir_page.bucket_ptr_count);
  for (auto &b : dir_page.bucket_ptrs)
    read_v(it, b);

  return dir_page;
}
