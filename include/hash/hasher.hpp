#pragma once

#include "buffer/buffer_manager.hpp"
#include "comparison.hpp"
#include "hash/bucket.hpp"
#include "hash/directory.hpp"
#include "reg_ptr.hpp"
#include "result_set.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>

class Hasher {
  BufferManager &buffer;
  const uint32_t null_page_id;

public:
  size_t bucket_capacity, directory_capacity;

  const uint8_t key_type{};
  const uint16_t key_size{};

  uint32_t directory_id;

  Hasher(BufferManager &buffer, uint32_t directory_id,
         size_t key_type, size_t key_size)
      : buffer(buffer), null_page_id(buffer.null_page_id()),
        directory_id(directory_id), key_size(key_size), key_type(key_type) {
    directory_capacity = calculate_directory_capacity();
    bucket_capacity = calculate_bucket_capacity();

    if (directory_id == null_page_id) {
      auto directory = allocate_directory();
      this->directory_id = directory.page_id;

      auto bucket = allocate_bucket(0, 0);
      directory.bucket_ptrs[0] = bucket.page_id;

      unload_bucket(bucket, true);
      unload_directory(directory, true);
    }
  }

  Hasher(BufferManager &buffer, uint32_t directory_id,
         size_t key_type, size_t key_size, size_t bucket_capacity)
      : buffer(buffer), null_page_id(buffer.null_page_id()),
        directory_id(directory_id), key_size(key_size), key_type(key_type),
        bucket_capacity(bucket_capacity) {
    directory_capacity = calculate_directory_capacity();

    if (directory_id == null_page_id) {
      auto directory = allocate_directory();
      this->directory_id = directory.page_id;

      auto bucket = allocate_bucket(0, 0);
      directory.bucket_ptrs[0] = bucket.page_id;

      unload_bucket(bucket, true);
      unload_directory(directory, true);
    }
  }

  uint32_t locate_bucket(const SQL_type_ &key);

  std::vector<RegPtr> search(Comparator &equals);
  std::vector<RegPtr> search(SQL_type_ &key);
  std::vector<RegPtr> search_in_bucket(uint32_t bucket_id, SQL_type_ &key);

  void insert(const SQL_type_ &key, const RegPtr &reg_ptr);
  void insert_non_full_bucket(Bucket &bucket, const SQL_type_ &key,
                              const RegPtr &reg_ptr);

  void insert_overflow_bucket(uint32_t bucket_id,
                              const SQL_type_ &key, const RegPtr &reg_ptr);

  void insert_from_set(const ResultSet &set, size_t key_col_index);

  void remove(SQL_type_ &key);
  void remove_in_bucket(uint32_t bucket_id, SQL_type_ &key);

  void update(SQL_type_ &old_key, SQL_type_ &new_key);

  void split_bucket(uint32_t bucket_id);

  DirectoryPage load_directory(uint32_t page_id);
  DirectoryPage allocate_directory();
  void unload_directory(DirectoryPage &directory, bool is_dirty = false);

  Bucket load_bucket(uint32_t page_id);
  Bucket allocate_bucket(uint32_t prefix, uint8_t local_depth);
  void unload_bucket(Bucket &bucket, bool is_dirty = false);

  size_t calculate_bucket_capacity();
  size_t calculate_directory_capacity();

  size_t hash_key(const SQL_type_ &key, size_t d);
  size_t hash_key_bits(std::vector<unsigned char> &bytes, size_t d);

  // Solo asigna punteros, no crea buckets
  void expand_directory(DirectoryPage &directory);

  bool check_bucket_keys_same_prefix(Bucket &bucket, size_t depth);
};
