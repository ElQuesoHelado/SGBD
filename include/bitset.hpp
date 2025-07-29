#pragma once

#include <climits>
#include <span>
#include <stdexcept>

class BitSet {
  std::span<unsigned char> buffer_;
  size_t num_bits_;

public:
  BitSet(std::span<unsigned char> buffer, size_t num_bits)
      : buffer_(buffer), num_bits_(num_bits) {
    if (buffer.size() < required_bytes()) {
      throw std::runtime_error("Buffer too small for BitSet");
    }
  }

  bool test(size_t pos) const {
    check_bounds(pos);
    const auto [byte_pos, bit_pos] = get_position(pos);
    return buffer_[byte_pos] & (1 << bit_pos);
  }

  void set(size_t pos, bool value = true) {
    check_bounds(pos);
    const auto [byte_pos, bit_pos] = get_position(pos);
    if (value) {
      buffer_[byte_pos] |= (1 << bit_pos);
    } else {
      buffer_[byte_pos] &= ~(1 << bit_pos);
    }
  }

  size_t size() const { return num_bits_; }
  std::string to_string() {
    std::string str{};
    for (size_t i{}; i < num_bits_; ++i)
      str += std::to_string(test(i));

    return str;
  }

private:
  size_t required_bytes() const {
    return (num_bits_ + CHAR_BIT - 1) / CHAR_BIT;
  }

  void check_bounds(size_t pos) const {
    if (pos >= num_bits_) {
      throw std::out_of_range("Bit position out of range");
    }
  }

  std::pair<size_t, size_t> get_position(size_t pos) const {
    return {pos / CHAR_BIT, pos % CHAR_BIT};
  }
};
