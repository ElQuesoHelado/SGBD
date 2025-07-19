#include "hash/hasher.hpp"
#include "hash/bucket.hpp"
#include "hash/directory.hpp"
#include "reg_ptr.hpp"
#include <cstddef>
#include <cstdint>

DirectoryPage Hasher::load_directory(uint32_t page_id) {
  auto &frame = buffer.load_pin_page(page_id);

  return {frame.page_bytes, page_id, null_page_id,
          calculate_directory_capacity()};
}

void Hasher::unload_directory(DirectoryPage &directory, bool is_dirty) {
  if (is_dirty) {
    auto &frame = buffer.load_pin_page(directory.page_id);
    auto page_it = frame.page_bytes.begin();

    directory.serialize(page_it);

    buffer.free_unpin_page(directory.page_id, true);
  }

  buffer.free_unpin_page(directory.page_id, false);
}

DirectoryPage Hasher::allocate_directory() {
  auto &frame = buffer.get_load_free_frame();
  return {frame.page_id, null_page_id, directory_capacity};
}

Bucket Hasher::load_bucket(uint32_t page_id) {
  auto &frame = buffer.load_pin_page(page_id);

  return {frame.page_bytes, page_id, bucket_capacity,
          key_type, key_size, null_page_id};
}

Bucket Hasher::allocate_bucket(uint32_t prefix, uint8_t local_depth) {
  auto &frame = buffer.get_load_free_frame();
  return {frame.page_id, prefix, bucket_capacity, local_depth,
          key_type, key_size, null_page_id};
}

void Hasher::unload_bucket(Bucket &bucket, bool is_dirty) {
  if (is_dirty) {
    auto &frame = buffer.load_pin_page(bucket.page_id);
    auto page_it = frame.page_bytes.begin();

    bucket.serialize(page_it);

    buffer.free_unpin_page(bucket.page_id, true);
  }

  buffer.free_unpin_page(bucket.page_id, false);
}
