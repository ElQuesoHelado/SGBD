#include "hash/hasher.hpp"
#include <cstddef>

void Hasher::update(const SQL_type_ &old_key, const SQL_type_ &new_key) {
  auto reg_ptrs = search(old_key);

  remove(old_key);

  for (auto &r : reg_ptrs)
    insert(new_key, r);
}

void Hasher::update_from_set(const ResultSet &old_set,
                             const ResultSet &new_set, size_t key_col_index) {
  if (old_set.registers.size() != new_set.registers.size())
    return;

  for (size_t i{}; i < old_set.registers.size(); ++i) {
    update(old_set.registers[i].values[key_col_index],
           new_set.registers[i].values[key_col_index]);
  }
}
