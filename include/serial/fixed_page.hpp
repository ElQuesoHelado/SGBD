#pragma once

#include "serial/fixed_data.hpp"

namespace serial {

struct FixedPage {
  serial::PageHeader page_header;
  serial::FixedDataHeader fixed_data_header;
  std::vector<unsigned char> &page_bytes;

  FixedPage(std::span<unsigned char> &buffer,
            std::vector<unsigned char> &page_bytes)
      : page_header(buffer),
        fixed_data_header(buffer),
        page_bytes(page_bytes) {
  }

  // Nueva pagina
  FixedPage(std::span<unsigned char> &buffer,
            std::vector<unsigned char> &page_bytes,
            size_t free_bytes, size_t reg_size, size_t max_n_regs)
      : page_header(buffer),
        fixed_data_header(buffer, free_bytes, reg_size, max_n_regs),
        page_bytes(page_bytes) {
    page_header.free_space = free_bytes;
  }

  size_t insert_register_bytes(std::vector<unsigned char> &reg_bytes) {
    size_t free_reg_pos = serial::find_free_reg_pos(fixed_data_header);
    return insert_register_bytes_on_pos(reg_bytes, free_reg_pos);
  }

  size_t insert_register_bytes_on_pos(
      std::vector<unsigned char> &reg_bytes, size_t free_reg_pos) {
    // Calculamos posicion donde insertar
    size_t byte_offset_free_reg = serial::calculate_reg_offset(fixed_data_header,
                                                               free_reg_pos);

    if (free_reg_pos >= fixed_data_header.max_n_regs) {
      throw std::runtime_error(
          "No hay registros libres en bitmap pero se intentó insertar");
    }

    // El write si procede
    fixed_data_header.free_bytes -= fixed_data_header.reg_size;
    fixed_data_header.free_register_bitmap->set(free_reg_pos, true);

    page_header.free_space -= fixed_data_header.reg_size;
    page_header.n_regs++;

    auto page_it = page_bytes.begin() + byte_offset_free_reg;

    // Copia registro como tal
    std::copy(reg_bytes.begin(),
              reg_bytes.end(), page_it);

    return free_reg_pos;
  }
};

} // namespace serial
