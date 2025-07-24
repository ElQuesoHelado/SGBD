#include "bptree/bptree.hpp"
#include "megatron.hpp"
#include "reg_ptr.hpp"
#include "result_set.hpp"
#include <cstddef>

std::vector<RegPtr> BPTree::search(Comparator &comp) {
  auto root = load_node(root_id);

  return search(root, comp);
}

std::vector<RegPtr> BPTree::search(BPNode &x, Comparator &comp) {
  size_t i{};
  while (i < x.n_keys && !comp.evaluate(x.keys[i])) {
    i++;
  }

  if (x.is_leaf) {
    // Se libera nodo internamente
    return linked_leaf_search(x, comp, i);
  } else {
    // One pass garantiza no ser utilizado otra vez
    auto child = load_node(x.ptrs[i]);

    unload_node(x);
    return search(child, comp);
  }
}

std::vector<RegPtr> BPTree::linked_leaf_search(
    BPNode &x, Comparator &comp, size_t i) {
  std::vector<RegPtr> reg_ptrs{};
  while (i < x.n_keys && comp.evaluate(x.keys[i])) {
    reg_ptrs.emplace_back(x.ptrs[i], x.reg_slots[i]);
    i++;
  }

  unload_node(x);

  if (i == x.n_keys && x.ptrs[i] != 0) {
    auto sibling = load_node(x.ptrs[i]);

    auto partial = linked_leaf_search(sibling, comp, 0);
    reg_ptrs.insert(reg_ptrs.end(), partial.begin(), partial.end());
  }

  return reg_ptrs;
}
