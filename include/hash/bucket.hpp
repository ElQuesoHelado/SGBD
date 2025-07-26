#pragma once
#include "reg_ptr.hpp"
#include "serial/generic.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

// Un bucket guarda sus propios datos
struct Bucket {
  uint32_t overflow_page{}, prefix{};
  uint8_t local_depth{};
  uint8_t size{};
  uint16_t capacity{}; // Dicta cuando se llena
  std::vector<SQL_type_> keys{};
  std::vector<RegPtr> reg_ptrs{};

  uint32_t page_id{}, null_page_id{};

  Bucket(std::vector<unsigned char> &bytes, uint32_t bucket_id,
         size_t capacity, size_t key_type, size_t key_size,
         uint32_t null_page_id)
      : page_id(bucket_id), capacity(capacity),
        null_page_id(null_page_id), overflow_page(null_page_id) {
    deserialize(bytes, key_type, key_size);
  }

  Bucket(uint32_t bucket_id, uint32_t prefix, size_t capacity,
         uint8_t local_depth, size_t key_type, size_t key_size,
         uint32_t null_page_id)
      : page_id(bucket_id), capacity(capacity), prefix(prefix),
        local_depth(local_depth), null_page_id(null_page_id),
        overflow_page(null_page_id) {
    auto empty =
        string_to_sql_type("", key_type, key_size);

    keys.resize(capacity, empty);
    reg_ptrs.resize(capacity, {null_page_id, 0});
  }

  template <typename Iter>
  void serialize(Iter &out_it) {
    write_v(out_it, overflow_page);
    write_v(out_it, prefix);
    write_v(out_it, local_depth);
    write_v(out_it, size); // Garantiza que nunca sea != de n_slots
    write_v(out_it, capacity);

    for (auto &k : keys) {
      auto type_bytes =
          serialize_sql_type(k);
      std::copy(type_bytes.begin(), type_bytes.end(), out_it);
      std::advance(out_it, type_bytes.size());
    }

    for (auto &r : reg_ptrs)
      write_v(out_it, r);
  }

  void deserialize(std::vector<unsigned char> &bytes,
                   size_t key_type, size_t key_size) {
    auto it = bytes.begin();

    deserialize(it, key_type, key_size);
  }

  template <typename Iter>
  void deserialize(Iter &in_it, size_t key_type, size_t key_size) {
    read_v(in_it, overflow_page);
    read_v(in_it, prefix);
    read_v(in_it, local_depth);
    read_v(in_it, size);
    read_v(in_it, capacity);

    auto empty =
        string_to_sql_type("", key_type, key_size);

    keys.resize(capacity, empty);
    reg_ptrs.resize(capacity, {0, 0});

    for (auto &k : keys)
      k = deserialize_sql_type(in_it, key_type, key_size);

    for (auto &r : reg_ptrs)
      read_v(in_it, r);
  }
};
