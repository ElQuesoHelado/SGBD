#include "disk_manager.hpp"
#include "megatron.hpp"
#include "serial/generic.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "types.hpp"
#include <string_view>
#include <type_traits>
#include <vector>

int main(int argc, char *argv[]) {
  uint32_t x = 13300000;
  std::vector<unsigned char> buffer(4);

  std::memcpy(&buffer.front(), reinterpret_cast<unsigned char *>(&x), sizeof(x));
  // std::memcpy(&value, &(*it), sizeof(T));

  // SQL_type var = static_cast<uint32_t>(x);

  auto it = buffer.begin();
  SQL_type var = deserialize_sql_type(it, 2, sizeof(uint32_t));
  std::cout << SQL_type_to_string(var) << std::endl;
  // SQL_type
}
