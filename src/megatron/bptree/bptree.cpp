#include "bptree/bptree.hpp"
#include "bptree/bpnode.hpp"
#include <cstddef>
#include <print>

BPNode BPTree::load_node(size_t page_id) {
  auto &frame = buffer.load_pin_page(page_id);

  // if (page_id == 7)
  //   std::println("aea");

  return BPNode(frame.page_bytes, key_type, key_size, page_id, min_degree);
}

void BPTree::unload_node(BPNode &node, bool is_dirty) {
  if (is_dirty) {
    // if (node.node_id == 7)
    //   std::println("aea");

    auto &frame = buffer.load_pin_page(node.node_id);
    auto page_it = frame.page_bytes.begin();

    // frame.page_bytes
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
