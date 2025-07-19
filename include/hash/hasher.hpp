#pragma once

#include "buffer/buffer_manager.hpp"
#include "comparison.hpp"
#include "hash/directory.hpp"
#include "reg_ptr.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>

class Hasher {
  BufferManager &buffer;
  const uint32_t null_page_id;

public:
  const size_t bucket_capacity, directory_capacity;

  const uint8_t key_type{};
  const uint16_t key_size{};

  uint32_t directory_id;

  Hasher(BufferManager &buffer, uint32_t directory_id,
         size_t key_type, size_t key_size)
      : buffer(buffer), null_page_id(buffer.null_page_id()),
        directory_id(directory_id),
        directory_capacity(calculate_directory_capacity()),
        bucket_capacity(calculate_bucket_capacity()),
        key_size(key_size), key_type(key_type) {}

  uint32_t locate_bucket(SQL_type &key);

  std::vector<RegPtr> search(Comparator &equals);
  std::vector<RegPtr> search_in_bucket(uint32_t bucket_id, SQL_type &key);

  void insert(SQL_type &key, RegPtr &reg_ptr);
  void insert_non_full_bucket(Bucket &bucket, SQL_type &key, RegPtr &reg_ptr);

  void insert_overflow_bucket(uint32_t bucket_id,
                              SQL_type &key, RegPtr &reg_ptr);

  void split_bucket(uint32_t bucket_id);

  DirectoryPage load_directory(uint32_t page_id);
  DirectoryPage allocate_directory();
  void unload_directory(DirectoryPage &directory, bool is_dirty = false);

  Bucket load_bucket(uint32_t page_id);
  Bucket allocate_bucket(uint32_t prefix, uint8_t local_depth);
  void unload_bucket(Bucket &bucket, bool is_dirty = false);

  size_t calculate_bucket_capacity();
  size_t calculate_directory_capacity();

  size_t hash_key(SQL_type &key, size_t d);
  size_t hash_key_bits(std::vector<unsigned char> &bytes, size_t d);

  bool check_bucket_keys_same_prefix(Bucket &bucket, size_t depth);
};
