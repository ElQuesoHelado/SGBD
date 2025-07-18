#include "reg_ptr.hpp"
#include "result_set.hpp"
#include "types.hpp"
#include <bitset>
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <print>
#include <stdexcept>
#include <unordered_map>
#include <vector>

class Bucket {
public:
  int localDepth;
  std::vector<std::pair<std::string, RegPtr>> records;

  Bucket(int depth) : localDepth(depth) {}

  bool isFull(int bucketSize) const { return records.size() >= bucketSize; }

  void insert(const std::string &binaryKey, const RegPtr value) {
    records.emplace_back(binaryKey, value);
  }

  bool canAcceptDuplicateKey(const std::string &binaryKey) const {
    for (const auto &[k, _] : records) {
      if (k != binaryKey)
        return false;
    }
    return true;
  }

  bool remove(const RegPtr &value) {
    auto it = std::remove_if(records.begin(), records.end(),
                             [&value](const auto &pair) { return pair.second == value; });

    if (it != records.end()) {
      records.erase(it, records.end());
      return true;
    }
    return false;
  }

  std::vector<RegPtr> search(const std::string &binaryKey) const {
    std::vector<RegPtr> result;
    for (const auto &[k, v] : records) {
      if (k == binaryKey)
        result.push_back(v);
    }
    return result;
  }
};

class Hasher {
private:
  int globalDepth;
  int bucketSize;
  std::vector<std::shared_ptr<Bucket>> directory;

  std::string toBinaryKey(const SQL_type &key) const {
    auto bytes = serialize_sql_type(key);

    std::string binaryKey;
    binaryKey.reserve(bytes.size() * 8); // Pre-reservar espacio para los bits

    for (unsigned char byte : bytes) {
      binaryKey += std::bitset<8>(byte).to_string();
    }

    return binaryKey;
    // return std::visit([this](auto &&arg) -> std::string {
    //   using T = std::decay_t<decltype(arg)>;
    //   if constexpr (std::is_same_v<T, int>) {
    //     return std::bitset<32>(arg).to_string();
    //   } else if constexpr (std::is_same_v<T, float>) {
    //     uint32_t bits;
    //     memcpy(&bits, &arg, sizeof(float));
    //     return std::bitset<32>(bits).to_string();
    //   } else if constexpr (std::is_same_v<T, std::string>) {
    //     std::string bin;
    //     for (char c : arg)
    //       bin += std::bitset<8>(c).to_string();
    //     return bin;
    //   } else if constexpr (std::is_same_v<T, bool>) {
    //     return arg ? "1" : "0";
    //   } else {
    //     // static_assert(std::always_false_v<T>, "Tipo no soportado");
    //     return "";
    //   }
    // },
    //                   key);
  }

  size_t hashKey(const std::string &binaryKey) const {
    return std::hash<std::string>{}(binaryKey);
  }

  int getDirectoryIndex(const std::string &binaryKey) const {
    size_t h = hashKey(binaryKey);
    return h & ((1 << globalDepth) - 1);
  }

  void splitBucket(int index) {
    auto oldBucket = directory[index];
    int localDepth = oldBucket->localDepth;

    if (localDepth == globalDepth) {
      globalDepth++;
      directory.resize(1 << globalDepth);
      for (int i = 0; i < (1 << (globalDepth - 1)); ++i)
        directory[i + (1 << (globalDepth - 1))] = directory[i];
    }

    auto newBucket = std::make_shared<Bucket>(localDepth + 1);
    oldBucket->localDepth++;

    int pattern = index & ((1 << localDepth) - 1);

    for (int i = 0; i < directory.size(); ++i) {
      if ((i & ((1 << localDepth) - 1)) == pattern && ((i >> localDepth) & 1)) {
        directory[i] = newBucket;
      }
    }

    auto tempRecords = oldBucket->records;
    oldBucket->records.clear();

    for (const auto &[k, v] : tempRecords)
      insertBinary(k, v);
  }

public:
  size_t hashed_col_index;

  Hasher(int bucketSize = 2, size_t col_idx = 0)
      : globalDepth(1), bucketSize(bucketSize),
        hashed_col_index(col_idx) {
    directory.resize(1 << globalDepth);
    for (auto &bucket : directory)
      bucket = std::make_shared<Bucket>(globalDepth);
  }

  void insert(const SQL_type &key, const RegPtr value) {
    std::string binKey = toBinaryKey(key);
    if (binKey.empty()) {
      std::cerr << "Error: clave inválida o tipo no soportado" << std::endl;
      return;
    }
    insertBinary(binKey, value);
  }

  void insertBinary(const std::string &binaryKey, const RegPtr value) {
    int index = getDirectoryIndex(binaryKey);
    auto bucket = directory[index];

    if (bucket->isFull(bucketSize)) {
      if (bucket->canAcceptDuplicateKey(binaryKey)) {
        bucket->insert(binaryKey, value);
        return;
      }
      splitBucket(index);
      insertBinary(binaryKey, value);
    } else {
      bucket->insert(binaryKey, value);
    }
  }

  std::vector<RegPtr> search(const SQL_type &key) const {
    std::string binKey = toBinaryKey(key);
    int index = getDirectoryIndex(binKey);
    return directory[index]->search(binKey);
  }

  bool remove(const SQL_type &key, const RegPtr value) {
    std::string binKey = toBinaryKey(key);
    int index = getDirectoryIndex(binKey);
    return directory[index]->remove(value);
  }

  // Métodos requeridos
  void indexarResultSet(const ResultSet &resultSet, const std::string &columna) {
    // Encontrar el índice de la columna a indexar
    size_t col_idx = -1;
    for (size_t i = 0; i < resultSet.columns.size(); ++i) {
      if (resultSet.columns[i] == columna) {
        col_idx = i;
        break;
      }
    }

    if (col_idx == -1) {
      throw std::runtime_error("Columna no encontrada en el ResultSet");
    }

    // Indexar todos los registros
    for (const auto &reg : resultSet.registers) {
      if (col_idx >= reg.values.size()) {
        continue; // O podrías lanzar un error
      }

      // Convertir el valor a string
      // std::string valor = SQL_type_to_string(reg.values[col_idx]);
      // std::println("{}", valor);

      // Insertar en el índice
      this->insert(reg.values[col_idx], RegPtr(reg.page_id, reg.position));
    }
  }

  void actualizarDesdeInsercion(const ResultSet &resultSet, const std::string &columna) {
    indexarResultSet(resultSet, columna);
  }

  void actualizarDesdeEliminacion(const ResultSet &resultSet, const std::string &columna) {
    size_t col_idx = -1;
    for (size_t i = 0; i < resultSet.columns.size(); ++i) {
      if (resultSet.columns[i] == columna) {
        col_idx = i;
        break;
      }
    }

    if (col_idx == -1) {
      throw std::runtime_error("Columna no encontrada en el ResultSet");
    }

    for (const auto &reg : resultSet.registers) {
      if (col_idx >= reg.values.size())
        continue;

      std::string valor = SQL_type_to_string(reg.values[col_idx]);
      this->remove(reg.values[col_idx], RegPtr(reg.page_id, reg.position));
    }
  }

  std::vector<RegPtr> buscarResultSet(const SQL_type &clave) {
    return this->search(clave);
  }

private:
  // Función auxiliar para obtener el valor de una columna (debes implementarla)
  SQL_type obtenerValorColumna(const auto &registro, const std::string &columna) {
    // Implementación específica de tu SGBD
    return SQL_type{}; // Placeholder
  }
};
