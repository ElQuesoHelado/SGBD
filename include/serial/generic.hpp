#pragma once
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <vector>

namespace serial {

// Set de tipos compartidos para toda serializacion
constexpr size_t
    NAME_SIZE{20},
    TYPE_SIZE{10};
} // namespace serial

/*
 * @brief Asigna un valor desde un byte buffer, avanza el iterador en size_of(T) bytes
 * @param Iter Iterador a primer byte a ser escrito
 * @param T Destino de bytes
 */
template <typename T, typename Iter>
inline void read_v(Iter &it, T &value) {
  static_assert(std::is_trivially_copyable_v<T>, "Read de bytes de tipo no trivial");

  std::memcpy(&value, &(*it), sizeof(T));
  std::advance(it, sizeof(T));
}

/*
 * @brief Append de bytes de un valor a un byte buffer
 * @param std::vector<unsigned char> Byte buffer
 * @param T Valor a ser escrito
 * @note Buffer deberia tener capacidad para evitar reallocation
 */
template <typename T>
inline void write_v(std::vector<unsigned char> &buffer, const T &value) {
  static_assert(std::is_trivially_copyable_v<T>, "Write de bytes de tipo no trivial");

  auto ptr = reinterpret_cast<const unsigned char *>(&value);
  buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
}

template <typename T, typename Iter>
inline void write_v(Iter &it, const T &value) {
  static_assert(std::is_trivially_copyable_v<T>, "Read de bytes de tipo no trivial");

  std::memcpy(&(*it), &value, sizeof(T));
  std::advance(it, sizeof(T));
}

template <size_t size>
inline std::string_view array_to_string_view(std::array<char, size> &array) {
  auto it = std::find(array.begin(), array.end(), '\0');
  std::size_t len = std::distance(array.begin(), it);
  std::string_view sv(reinterpret_cast<const char *>(array.data()), len);
  return sv;
}
