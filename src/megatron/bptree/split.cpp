#include "bptree/bpnode.hpp"
#include "bptree/bptree.hpp"
#include <cstddef>
#include <print>

// x nodo padre NO lleno
void BPTree::split_child(BPNode &x, int i) {
  auto y = load_node(x.ptrs[i]);
  auto z = allocate_node();

  // Shifts para promovido
  for (int j = static_cast<int>(x.n_keys); j >= i + 1; --j)
    x.ptrs[j + 1] = x.ptrs[j];

  x.ptrs[i + 1] = z.node_id;

  for (int j = static_cast<int>(x.n_keys) - 1; j >= i; --j)
    x.keys[j + 1] = x.keys[j];

  x.n_keys++;

  z.is_leaf = y.is_leaf;
  // Z contiene el nodo promovido
  if (y.is_leaf) {
    // Esto por equivalencia 1 a 1 entre key y puntero a registro
    for (int j = 0; j < min_degree - 1; ++j) {
      z.keys[j] = y.keys[j + min_degree];
      z.ptrs[j] = y.ptrs[j + min_degree];
    }

    z.n_keys = min_degree - 1;
    y.n_keys = min_degree;

    // Enlazar hojas
    z.ptrs[z.n_keys] = y.ptrs[y.n_keys + 1];
    y.ptrs[y.n_keys] = z.node_id;

    // Primera clave de z al padre
    x.keys[i] = z.keys[0];

  } else {
    // Mover claves y punteros a z
    for (int j = 0; j < min_degree - 1; ++j)
      z.keys[j] = y.keys[j + min_degree];
    for (int j = 0; j < min_degree; ++j)
      z.ptrs[j] = y.ptrs[j + min_degree];

    // Promocionar mediana al padre
    x.keys[i] = y.keys[min_degree - 1];
    y.n_keys = min_degree - 1;
    z.n_keys = min_degree - 1;
  }

  unload_node(y, true);
  unload_node(z, true);
  unload_node(x, true);
}

BPNode BPTree::split_root() {
  auto s = allocate_node();
  s.is_leaf = false;
  s.ptrs[0] = root_id;

  root_id = s.node_id;

  split_child(s, 0);

  return s;
}
