#pragma once

#include "bpnode.hpp"
#include "buffer/buffer_manager.hpp"
#include "comparison.hpp"
// #include "megatron.hpp"
#include "reg_ptr.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

class BPTree {
  BufferManager &buffer;

  uint32_t null_page_id;

  serial::TableMetadata &table_metadata;

public:
  const uint8_t key_type{};
  const uint16_t key_size{}; // De acuedo a SQL_type
  uint32_t root_id;
  size_t min_degree; // TODO: determinado en block_size

  BPTree(BufferManager &buffer,
         serial::TableMetadata &table_metadata,
         uint32_t root_id,
         size_t min_degree, size_t key_type, size_t key_size)
      : buffer(buffer), table_metadata(table_metadata),
        null_page_id(buffer.null_page_id()), root_id(root_id),
        min_degree(min_degree), key_type(key_type), key_size(key_size) {
    if (root_id == null_page_id) {
      auto node = allocate_node();
      this->root_id = node.node_id;
      unload_node(node, true);
    }
  }

  // ~BPTree() {
  // }
  void print_tree(uint32_t page_id, size_t depth = 0);

  std::vector<RegPtr> search(Comparator &comp);
  std::vector<RegPtr> search(BPNode &x, Comparator &comp);

  void remove(const SQL_type_ &key);
  void remove(BPNode &x, const SQL_type_ &key);

  void insert(const SQL_type_ &key, const RegPtr reg_ptr);
  void insert_non_full(BPNode &x, const SQL_type_ &key, const RegPtr &reg_ptr);

  // Lineal, hasta que rompa condicion
  std::vector<RegPtr> linked_leaf_search(BPNode &x, Comparator &comp, size_t i);

  void split_child(BPNode &x, int i);
  BPNode split_root();

  BPNode load_node(uint32_t page_id);
  BPNode allocate_node();
  void unload_node(BPNode &node, bool is_dirty = false);
};
