#include "bptree/bpnode.hpp"
#include "bptree/bptree.hpp"
#include <cstddef>
#include <print>

// x nodo padre NO lleno
void BPTree::split_child(BPNode &x, int i) {
  auto y = load_node(x.ptrs[i]);
  auto z = allocate_node();

  z.is_leaf = y.is_leaf;

  // Z[0] contiene el nodo promovido
  if (y.is_leaf) {
    for (int j = 0; j < min_degree; ++j) {
      z.keys[j] = y.keys[j + min_degree - 1];
      z.reg_slots[j] = y.reg_slots[j + min_degree - 1];
    }

    for (int j = 0; j < min_degree + 1; ++j)
      z.ptrs[j] = y.ptrs[j + min_degree - 1];

    // Enlazar hojas
    z.n_keys = min_degree;
    z.ptrs[z.n_keys] = y.ptrs[y.n_keys];

    y.n_keys = min_degree - 1;
    y.ptrs[y.n_keys] = z.node_id;

  } else {
    for (int j = 0; j < min_degree - 1; ++j)
      z.keys[j] = y.keys[j + min_degree];
    for (int j = 0; j < min_degree; ++j)
      z.ptrs[j] = y.ptrs[j + min_degree];

    y.n_keys = min_degree - 1;
    z.n_keys = min_degree - 1;
  }

  // i+1 : ptr a z
  for (int j = static_cast<int>(x.n_keys); j >= i + 1; --j)
    x.ptrs[j + 1] = x.ptrs[j];

  x.ptrs[i + 1] = z.node_id;

  // i : key promovida
  for (int j = static_cast<int>(x.n_keys) - 1; j >= i; --j)
    x.keys[j + 1] = x.keys[j];

  x.keys[i] = y.keys[min_degree - 1];

  x.n_keys++;

  unload_node(y, true);
  unload_node(z, true);
  unload_node(x, true);
}

BPNode BPTree::split_root() {
  auto r = load_node(root_id);
  auto s = allocate_node();

  s.is_leaf = r.is_leaf;
  r.is_leaf = false;

  s.n_keys = r.n_keys;
  s.keys = r.keys;
  s.ptrs = r.ptrs;
  s.reg_slots = r.reg_slots;

  r.ptrs[0] = s.node_id;
  r.n_keys = 0;

  unload_node(s, true);
  // root_id = s.node_id;

  split_child(r, 0);

  return r;
}
