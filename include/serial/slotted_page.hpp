#pragma once

#include "serial/fixed_data.hpp"
#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace serial {

struct SlottedPage {
  serial::PageHeader page_header;
  serial::SlottedDataHeader slotted_data_header;
  std::vector<unsigned char> &page_bytes;
  std::span<unsigned char> shrink_data;

  SlottedPage(std::span<unsigned char> &buffer, std::vector<unsigned char> &page_bytes)
      : page_header(buffer),
        slotted_data_header(buffer),
        page_bytes(page_bytes) {
    shrink_data = buffer;
  }

  size_t insert_register_bytes(std::vector<unsigned char> &reg_bytes) {
    // Se actualiza headers para aceptar un registro nuevo
    size_t free_slot = slotted_data_header.get_free_slot();
    if (free_slot == slotted_data_header.n_slots)
      free_slot = slotted_data_header.add_free_slot(page_header, shrink_data);

    size_t byte_offset_free_reg =
        serial::prepare_slotted_insert(slotted_data_header,
                                       free_slot,
                                       reg_bytes.size());

    page_header.free_space -= reg_bytes.size();
    page_header.n_regs++;

    // Insercion de registro en offset correcto
    auto page_it = page_bytes.begin() + byte_offset_free_reg;
    std::copy(reg_bytes.begin(), reg_bytes.end(), page_it);

    return free_slot;
  }
};
} // namespace serial
