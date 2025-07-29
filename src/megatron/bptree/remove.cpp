#include "bptree/bptree.hpp"

void BPTree::remove(const SQL_type_ &key) {
  auto root = load_node(root_id);

  return remove(root, key);
}

void BPTree::remove(BPNode &x, const SQL_type_ &key) {
  size_t i{};
  while (i < x.n_keys && key > x.keys[i]) {
    i++;
  }

  if (x.is_leaf) {
    if (key != x.keys[i])
      return;

    // Shift de valores
    while (i < x.n_keys - 1) {
      x.keys[i] = x.keys[i + 1];
      x.ptrs[i] = x.ptrs[i + 1];
      x.reg_slots[i] = x.reg_slots[i + 1];
      ++i;
    }

    x.ptrs[i] = x.ptrs[i + 1];

    x.n_keys--;
    unload_node(x, true);
  } else {
    // One pass garantiza no ser utilizado otra vez
    auto child = load_node(x.ptrs[i]);

    unload_node(x);
    return remove(child, key);
  }
}
