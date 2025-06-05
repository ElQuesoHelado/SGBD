#pragma once

#include "serial/generic.hpp"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

const std::unordered_map<std::string, uint16_t, std::hash<std::string>,
                         std::equal_to<>>
    type_to_size{
        {"TINYINT", sizeof(int8_t)},
        {"SMALLINT", sizeof(int16_t)},
        {"INTEGER", sizeof(int32_t)},
        {"BIGINT", sizeof(int64_t)},
        {"FLOAT", sizeof(float)},
        {"DOUBLE", sizeof(double)},
        {"CHAR", 0},
        {"VARCHAR", 0}, // 1 byte minimo para determinar tamanio
    };

const std::unordered_map<std::string, uint8_t, std::hash<std::string>,
                         std::equal_to<>>
    type_to_index{
        {"TINYINT", 0},
        {"SMALLINT", 1},
        {"INTEGER", 2},
        {"BIGINT", 3},
        {"FLOAT", 4},
        {"DOUBLE", 5},
        {"CHAR", 6},
        {"VARCHAR", 7},
    };

enum SQL_type_tag : uint8_t {
  TINYINT,
  SMALLINT,
  INTEGER,
  BIGINT,
  FLOAT,
  DOUBLE,
  CHAR,
  VARCHAR
};

struct CharType {
  std::string value;
  uint16_t length;

  // TODO: se deberia hacer un trim de padding por la derecha para comparar?,
  // al comparar con un chartype ambos se comparan con padding
  bool operator==(const CharType &other) const {
    return value == other.value;
  }

  CharType(std::string val, size_t len) : length(len) {
    if (val.size() < len)
      val.append(len - val.size(), ' ');
    else if (val.size() > len)
      val = val.substr(0, len);
    value = std::move(val);
  }
};

struct VarcharType {
  std::string value;
  uint8_t max_length;

  bool operator==(const VarcharType &other) const {
    return value == other.value;
  }

  VarcharType(std::string val, size_t maxLen) : max_length(maxLen) {
    if (val.size() > maxLen)
      val = val.substr(0, maxLen);
    value = std::move(val);
  }
};

// TODO: cambio a clase, agrupar to_string, length
typedef std::variant<int8_t, int16_t, int32_t, int64_t, float, double, CharType, VarcharType> SQL_type;

template <typename T>
concept StringCastable =
    requires(T val) {
      { std::to_string(val) } -> std::convertible_to<std::string>;
    };

// TODO: cambio a StringSimilar o algo asi, caso char*
template <typename T>
concept StringSame = std::is_same_v<std::remove_cvref_t<T>, std::string>;

template <typename T>
concept StringWrapper = requires(T a) {
  { a.value } -> std::convertible_to<std::string>;
};

template <typename T>
concept NumericType = std::integral<T> || std::floating_point<T>;

template <typename... Ts>
bool constexpr is_variant_numeric(const std::variant<Ts...> &var) {
  return std::visit([](const auto &arg) {
    return NumericType<std::decay_t<decltype(arg)>>;
  },
                    var);
}

template <typename... Ts>
bool constexpr is_variant_textual(const std::variant<Ts...> &var) {
  return std::visit([](const auto &arg) {
    return StringSame<std::decay_t<decltype(arg)>>;
  },
                    var);
}

template <typename T>
concept NumericSQL_type = requires(T var) {
  { is_variant_numeric(var) } -> std::convertible_to<bool>;
  requires is_variant_textual(var);
};

template <typename T>
concept StringSQL_type = requires(T var) {
  { is_variant_textual(var) } -> std::convertible_to<bool>;
  requires is_variant_textual(var);
};

// Patron overload para visit mas limpio
template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};

inline std::string SQL_type_to_string(SQL_type &field) {
  return std::visit(overload{
                        [](StringCastable auto &arg) { return std::to_string(arg); },
                        [](StringWrapper auto &arg) { return arg.value; }},
                    field);
}

// TODO: cambiar a optional caso sea nulo
inline SQL_type string_to_sql_type(std::string input, uint8_t type, uint16_t size) {
  try {
    switch (type) {
    case TINYINT: {
      if (input.empty())
        input = "0";
      int val = std::stoi(input);
      if (val < INT8_MIN || val > INT8_MAX)
        throw std::out_of_range("TINYINT out of range");
      return static_cast<int8_t>(val);
    }
    case SMALLINT: {
      if (input.empty())
        input = "0";
      int val = std::stoi(input);
      if (val < INT16_MIN || val > INT16_MAX)
        throw std::out_of_range("SMALLINT out of range");
      return static_cast<int16_t>(val);
    }
    case INTEGER: {
      if (input.empty())
        input = "0";
      return static_cast<int32_t>(std::stol(input));
    }
    case BIGINT: {
      if (input.empty())
        input = "0";
      return static_cast<int64_t>(std::stoll(input));
    }
    case FLOAT: {
      if (input.empty())
        input = "0";
      return static_cast<float>(std::stof(input));
    }
    case DOUBLE: {
      if (input.empty())
        input = "0";
      return static_cast<double>(std::stod(input));
    }
    case CHAR: {
      return CharType{input, size};
    }
    case VARCHAR: {
      return VarcharType{input, size};
    }
    default:
      throw std::runtime_error("Tipo para SQL_type invalido");
    }
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("string invalida para SQL_type ") + e.what());
  }
}

template <typename Iter>
inline SQL_type deserialize_sql_type(Iter &it, uint8_t type, size_t size) {
  auto read_bytes = [&](size_t count) -> std::vector<uint8_t> {
    std::vector<uint8_t> buffer;
    for (size_t i = 0; i < count; ++i, ++it) {
      buffer.push_back(*it);
    }
    return buffer;
  };

  switch (type) {
  case TINYINT: {
    int8_t val = static_cast<int8_t>(*it++);
    return val;
  }

  case SMALLINT: {
    auto bytes = read_bytes(2);
    int16_t val;
    std::memcpy(&val, bytes.data(), sizeof(int16_t));
    return val;
  }

  case INTEGER: {
    auto bytes = read_bytes(4);
    int32_t val;
    std::memcpy(&val, bytes.data(), sizeof(int32_t));
    return val;
  }

  case BIGINT: {
    auto bytes = read_bytes(8);
    int64_t val;
    std::memcpy(&val, bytes.data(), sizeof(int64_t));
    return val;
  }

  case FLOAT: {
    auto bytes = read_bytes(4);
    float val;
    std::memcpy(&val, bytes.data(), sizeof(float));
    return val;
  }

  case DOUBLE: {
    auto bytes = read_bytes(8);
    double val;
    std::memcpy(&val, bytes.data(), sizeof(double));
    return val;
  }

  case CHAR: {
    auto bytes = read_bytes(size);
    std::string str(reinterpret_cast<const char *>(bytes.data()), size);
    return CharType{str, size};
  }

  case VARCHAR: {
    uint8_t len = *it++;
    auto bytes = read_bytes(len);
    std::string str(reinterpret_cast<const char *>(bytes.data()), len);
    return VarcharType{str, size}; // `size` es el maxLength permitido
  }

  default:
    throw std::runtime_error("Tipo no definido para deserializar SQL_type");
  }
}

inline std::vector<unsigned char> serialize_sql_type(const SQL_type &value) {
  std::vector<unsigned char> output;

  if (value.index() == 2) {
    int32_t val = std::get<int32_t>(value);
    const unsigned char *ptr = reinterpret_cast<const unsigned char *>(&val);
    output.insert(output.end(), ptr, ptr + sizeof(int32_t));
    return output;
  }

  std::visit([&](const auto &val) {
    using T = std::decay_t<decltype(val)>;

    if constexpr (std::is_same_v<T, CharType>) {
      // CHAR: write exactly `length` bytes
      std::string padded = val.value;
      if (padded.size() < val.length)
        padded.append(val.length - padded.size(), ' ');
      else if (padded.size() > val.length)
        padded = padded.substr(0, val.length);

      output.insert(output.end(), padded.begin(), padded.begin() + val.length);
    } else if constexpr (std::is_same_v<T, VarcharType>) {
      // VARCHAR: 1 byte for length, then raw chars
      if (val.value.size() > 255)
        throw std::runtime_error("VARCHAR length exceeds 255");

      output.push_back(static_cast<unsigned char>(val.value.size()));
      output.insert(output.end(), val.value.begin(), val.value.end());
    } else {
      // Numeric types: copy raw bytes
      const unsigned char *ptr = reinterpret_cast<const unsigned char *>(&val);
      output.insert(output.end(), ptr, ptr + sizeof(T));
    }
  },
             value);

  return output;
}

// size_t get_length_field(StringCastable auto &field) {
//   return std::to_string(field).length();
// }
// size_t get_length_field(StringSame auto &field) {
//   return field.length();
//
// }

inline size_t SQL_type_length(SQL_type &field) {
  return std::visit(overload{[](StringCastable auto &arg) { return std::to_string(arg).length(); },
                             [](StringWrapper auto &arg) { return arg.value.length(); }},
                    field);
}
