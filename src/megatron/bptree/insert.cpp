#include "bptree/bptree.hpp"
#include "types.hpp"
#include <print>

void BPTree::insert(const SQL_type_ &key, const RegPtr reg_ptr) {
  if (SQL_type_to_string(key) == "7")
    std::print("a");

  auto r = load_node(root_id);

  if (r.n_keys == (2 * min_degree - 1)) {
    unload_node(r);
    auto s = split_root();
    insert_non_full(s, key, reg_ptr);
  } else {
    insert_non_full(r, key, reg_ptr);
  }
}

void BPTree::insert_non_full(BPNode &x,
                             const SQL_type_ &key, const RegPtr &reg_ptr) {
  auto i = x.n_keys - 1;
  if (x.is_leaf) {
    while (i >= 0 && key < x.keys[i]) {
      x.keys[i + 1] = x.keys[i];
      i--;
    }
    x.keys[i + 1] = key;
    x.ptrs[i + 1] = reg_ptr.page_id;
    x.reg_slots[i + 1] = reg_ptr.slot;
    x.n_keys++;

    unload_node(x, true);
  } else {
    while (i >= 0 && key < x.keys[i])
      i--;
    i++;

    auto child = load_node(x.ptrs[i]);

    if (child.n_keys == (2 * min_degree - 1)) {
      split_child(x, i);

      unload_node(child);

      if (key > x.keys[i])
        i++;

      child = load_node(x.ptrs[i]);

    } else {
      unload_node(x);
    }

    insert_non_full(child, key, reg_ptr);
  }
}
