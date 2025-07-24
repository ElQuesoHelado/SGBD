#pragma once

#include "serial/generic.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <vector>

struct BPNode {
  uint8_t is_leaf{true};
  uint16_t n_keys{};

  std::vector<SQL_type> keys{};

  // Generico, a nodo interno, heapfile page
  // o a la siguiente hoja(ultimo puntero)
  // (key1, key2)
  // (hijo1, hijo2, hijo3)
  // (heap1, heap2, hermano)
  std::vector<uint32_t> ptrs{};

  std::vector<uint16_t> reg_slots{}; // Posicion exacta registro

  uint32_t node_id{};
  size_t min_degree{};

  BPNode(std::vector<unsigned char> &bytes, size_t key_type,
         size_t key_size, uint32_t node_id, size_t min_degree)
      : node_id(node_id), min_degree(min_degree) {
    deserialize(bytes, key_type, key_size, min_degree);
  }

  BPNode(uint32_t node_id, size_t key_type, size_t key_size,
         size_t min_degree) : node_id(node_id), min_degree(min_degree) {
    size_t max_keys = 2 * min_degree - 1;
    size_t max_ptrs = 2 * min_degree;

    auto empty = string_to_sql_type("", key_type, key_size);

    keys.resize(max_keys, empty);
    ptrs.resize(max_ptrs);
    reg_slots.resize(max_keys);
  }

  BPNode();

  ~BPNode() {}

  std::vector<unsigned char> serialize() {
    std::vector<unsigned char> bytes;

    write_v(bytes, is_leaf);
    write_v(bytes, n_keys);

    for (auto &k : keys) {
      auto type_bytes = serialize_sql_type(k);
      bytes.insert(bytes.end(),
                   type_bytes.begin(), type_bytes.end());
    }

    for (auto &p : ptrs)
      write_v(bytes, p);

    if (is_leaf) {
      for (auto &s : reg_slots)
        write_v(bytes, s);
    }

    return bytes;
  }

  template <typename Iter>
  void serialize(Iter &out_it) {
    write_v(out_it, is_leaf);
    write_v(out_it, n_keys);

    for (auto &k : keys) {
      auto type_bytes = serialize_sql_type(k);
      std::copy(type_bytes.begin(), type_bytes.end(), out_it);
      std::advance(out_it, type_bytes.size());
    }

    for (auto &p : ptrs)
      write_v(out_it, p);

    if (is_leaf) {
      for (auto &s : reg_slots)
        write_v(out_it, s);
    }
  }

  void deserialize(std::vector<unsigned char> &bytes,
                   size_t key_type, size_t key_size, size_t min_degree) {
    auto it = bytes.begin();

    size_t max_keys = 2 * min_degree - 1;
    size_t max_ptrs = 2 * min_degree;

    auto empty = string_to_sql_type("", key_type, key_size);

    keys.resize(max_keys, empty);
    ptrs.resize(max_ptrs);
    reg_slots.resize(max_keys);

    read_v(it, is_leaf);
    read_v(it, n_keys);

    // keys.resize(n_keys);
    for (size_t i{}; i < max_keys; ++i) {
      keys[i] = deserialize_sql_type(it, key_type, key_size);
    }

    // ptrs.resize(n_keys + 1);
    for (size_t i{}; i < max_ptrs; ++i) {
      read_v(it, ptrs[i]);
    }

    if (is_leaf) {
      // reg_slots.resize(n_keys);
      for (size_t i{}; i < max_keys; ++i)
        read_v(it, reg_slots[i]);
    }
  }
};
