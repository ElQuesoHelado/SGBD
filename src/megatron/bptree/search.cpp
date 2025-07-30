#include "bptree/bptree.hpp"
#include "comparison.hpp"
#include <print>

std::vector<RegPtr> BPTree::search(Comparator &comp) {
  Comparator low = comp.get_low(), high = comp.get_high();

  auto root = load_node(root_id);

  std::println("Arbol con root en: {}", root_id);

  return search(root, low, high);
}

std::vector<RegPtr> BPTree::search(BPNode &x, Comparator &low, Comparator &high) {
  std::println("Buscando en nodo: {}", x.node_id);

  size_t i{};
  while (i < x.n_keys && !low.evaluate(x.keys[i])) {
    i++;
  }

  if (x.is_leaf) {
    // Se libera nodo internamente
    std::println("Se llego a una hoja, se busca secuencialmente");
    return linked_leaf_search(x, high, i);
  } else {
    // One pass garantiza no ser utilizado otra vez
    auto child = load_node(x.ptrs[i]);

    unload_node(x);
    return search(child, low, high);
  }
}

std::vector<RegPtr> BPTree::linked_leaf_search(
    BPNode &x, Comparator &comp, size_t i) {
  std::vector<RegPtr> reg_ptrs{};
  std::println("Nodo hoja: {}", x.node_id);
  // std::print("Registros encontrados en: ");
  while (i < x.n_keys && comp.evaluate(x.keys[i])) {
    std::print("({}, {}) ", x.ptrs[i], x.reg_slots[i]);
    reg_ptrs.emplace_back(x.ptrs[i], x.reg_slots[i]);
    i++;
  }
  std::println();

  unload_node(x);

  if (i == x.n_keys && x.ptrs[i] != 0) {
    auto sibling = load_node(x.ptrs[i]);

    auto partial = linked_leaf_search(sibling, comp, 0);
    reg_ptrs.insert(reg_ptrs.end(), partial.begin(), partial.end());
  }

  return reg_ptrs;
}
