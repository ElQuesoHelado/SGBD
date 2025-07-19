#include <cstddef>
#include <cstdint>

#include "hash/directory.hpp"
#include "hash/hasher.hpp"
#include "types.hpp"

size_t Hasher::hash_key(SQL_type &key, size_t d) {
  auto key_bytes = serialize_sql_type(key);
  return hash_key_bits(key_bytes, d);
}

// Encuentra hash en base a "d" bits menos significativos
// Necesariamente no podemos exceder de 32 bits de profundidad(size maximo de size_t)
size_t Hasher::hash_key_bits(std::vector<unsigned char> &bytes, size_t d) {
  size_t hash_value = 0;
  size_t bytes_to_copy = std::min(bytes.size(), sizeof(size_t));

  for (size_t i = 0; i < bytes_to_copy; ++i) {
    hash_value = (hash_value << 8) | bytes[i];
  }

  uint64_t mask = (d >= 32) ? ~0UL : (1UL << d) - 1;

  return hash_value & mask;
}

size_t Hasher::calculate_bucket_capacity() {
  size_t remm_bytes =
      buffer.page_size() - sizeof(Bucket::overflow_page) -
      sizeof(Bucket::local_depth) - sizeof(Bucket::size) -
      sizeof(Bucket::capacity);

  return remm_bytes / (key_size + sizeof(RegPtr));
}

size_t Hasher::calculate_directory_capacity() {
  size_t remm_bytes =
      buffer.page_size() - sizeof(DirectoryPage::next_page) -
      sizeof(DirectoryPage::global_depth);

  return remm_bytes / (sizeof(uint32_t));
}

uint32_t Hasher::locate_bucket(SQL_type &key) {
  auto directory = load_directory(directory_id);
  auto key_bytes =
      serialize_sql_type(key);

  size_t bucket_idx =
      hash_key_bits(key_bytes, directory.global_depth);

  // Ubicar directorio, puede estar en overflow
  while (bucket_idx >= directory_capacity &&
         directory.next_page != null_page_id) {
    unload_directory(directory);

    directory = load_directory(directory.next_page);

    bucket_idx -= directory_capacity;
  }

  if (bucket_idx >= directory_capacity ||
      directory.bucket_ptrs[bucket_idx] == null_page_id)
    return null_page_id;

  unload_directory(directory);

  return directory.bucket_ptrs[bucket_idx];
}

bool Hasher::check_bucket_keys_same_prefix(Bucket &bucket, size_t depth) {
  if (bucket.size == 0)
    return true;

  size_t i{};
  bool same_prefix{true};

  while (bucket.reg_ptrs[i].page_id == null_page_id) {
    ++i;
  }

  auto prefix = hash_key(bucket.keys[i], depth);

  for (size_t j{i + 1}; j < bucket.capacity; ++j) {
    if (bucket.reg_ptrs[j].page_id != null_page_id &&
        prefix != hash_key(bucket.keys[j], depth)) {
      same_prefix = false;
      break;
    }
  }

  return same_prefix;
}
