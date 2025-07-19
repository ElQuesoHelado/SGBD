#include "bptree/bptree.hpp"
#include "bptree/bpnode.hpp"
#include <cstddef>
#include <cstdint>
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
