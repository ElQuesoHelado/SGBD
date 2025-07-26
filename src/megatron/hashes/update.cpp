#include "hash/hasher.hpp"
#include "types.hpp"
#include <cstddef>

void Hasher::update(SQL_type_ &old_key, SQL_type_ &new_key) {
  auto reg_ptrs = search(old_key);

  remove(old_key);

  for (auto &r : reg_ptrs)
    insert(new_key, r);
}
