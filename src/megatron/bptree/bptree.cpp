#include "bptree/bptree.hpp"
#include <print>

BPNode BPTree::load_node(uint32_t page_id) {
  auto &frame = buffer.load_pin_page(page_id);

  return {frame.page_bytes, key_type, key_size,
          page_id, min_degree};
}

void BPTree::unload_node(BPNode &node, bool is_dirty) {
  if (is_dirty) {
    auto &frame = buffer.load_pin_page(node.node_id);
    auto page_it = frame.page_bytes.begin();

    node.serialize(page_it);

    buffer.free_unpin_page(node.node_id, true);
  }

  buffer.free_unpin_page(node.node_id, false);
}

// Crea y carga nuevo nodo
BPNode BPTree::allocate_node() {
  auto &frame = buffer.get_load_free_frame();
  return {frame.page_id, key_type, key_size, min_degree};
}

void BPTree::print_tree(uint32_t page_id, size_t depth) {
  for (size_t i{}; i < depth; ++i)
    std::print("  ");

  auto node = load_node(page_id);
  for (size_t i{}; i < node.n_keys; ++i) {
    std::print("{}, ", SQL_type_to_string(node.keys[i]));
  }
  std::println("\n");

  unload_node(node);
  if (!node.is_leaf) {
    for (size_t i{}; i <= node.n_keys; ++i)
      print_tree(node.ptrs[i], depth + 1);
  }
}
