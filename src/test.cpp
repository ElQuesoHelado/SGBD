#include "disk_manager.hpp"
#include "megatron.hpp"
#include "serial/generic.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include <string_view>
#include <type_traits>
#include <vector>

int main(int argc, char *argv[]) {
  // std::string str = "100101010110";
  // boost::dynamic_bitset<unsigned char> set(str);
  std::array<char, 5> array = {'\0', '\0', '\0', '\0', '\0'};

  if (array_to_string_view(array).empty())
    std::cout << "Vacio" << std::endl;
}
