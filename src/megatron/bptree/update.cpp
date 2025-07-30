#include "bptree/bptree.hpp"
#include "comparison.hpp"

void BPTree::update(const SQL_type_ &old_key, const SQL_type_ &new_key) {
  Comparator comp;
  comp.greater_equal_than(old_key)
      .AND()
      .less_equal_than(old_key);

  auto reg_ptrs = search(comp);

  remove(old_key);

  for (auto &r : reg_ptrs)
    insert(new_key, r);
}

void BPTree::update_from_set(
    const ResultSet &old_set,
    const ResultSet &new_set, size_t key_col_index) {
  remove_from_set(old_set, key_col_index);
  insert_from_set(new_set, key_col_index);
}
