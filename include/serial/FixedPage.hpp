#pragma once

#include "serial/fixed_data.hpp"
#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <vector>

struct FixedPage {
  serial::PageHeader page_header;
  serial::FixedDataHeader data_header;

  // std::vector<>
};
