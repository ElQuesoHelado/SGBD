#include "result_set.hpp"
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

// Estructura que representa un puntero a un registro
struct Reg_ptr {
  size_t pagina; // Número de página donde está el registro
  size_t slot;   // Número de slot dentro de la página

  Reg_ptr(size_t p, size_t s) : pagina(p), slot(s) {}

  bool operator==(const Reg_ptr &other) const {
    return pagina == other.pagina && slot == other.slot;
  }
};

// Bucket para el extendible hashing (ahora con soporte para overflow)
class Bucket_ {
private:
  size_t localDepth;
  std::list<std::pair<size_t, Reg_ptr>> entradas;
  std::shared_ptr<Bucket_> overflowBucket; // Puntero al bucket de overflow

public:
  explicit Bucket_(size_t depth) : localDepth(depth), overflowBucket(nullptr) {}

  // Inserta una entrada en este bucket o en su overflow
  void insertar(size_t hash, const Reg_ptr &regPtr, size_t tamBucket) {
    if (entradas.size() < tamBucket) {
      entradas.emplace_back(hash, regPtr);
    } else {
      // Si no hay overflow bucket, creamos uno
      if (!overflowBucket) {
        overflowBucket = std::make_shared<Bucket_>(localDepth);
      }
      overflowBucket->insertar(hash, regPtr, tamBucket);
    }
  }

  // Elimina una entrada del bucket o de sus overflows
  bool eliminar(const Reg_ptr &regPtr) {
    // Buscamos en las entradas principales
    for (auto it = entradas.begin(); it != entradas.end(); ++it) {
      if (it->second == regPtr) {
        entradas.erase(it);

        // Si hay overflow, movemos la primera entrada de overflow aquí
        if (overflowBucket && !overflowBucket->entradas.empty()) {
          entradas.splice(entradas.end(), overflowBucket->entradas,
                          overflowBucket->entradas.begin());

          // Si el overflow bucket queda vacío, lo eliminamos
          if (overflowBucket->entradas.empty() &&
              !overflowBucket->overflowBucket) {
            overflowBucket.reset();
          }
        }
        return true;
      }
    }

    // Si no se encontró en las entradas principales, buscamos en los overflows
    if (overflowBucket) {
      return overflowBucket->eliminar(regPtr);
    }

    return false;
  }

  // Obtiene todas las entradas (incluyendo los overflows)
  void obtenerTodasEntradas(std::list<std::pair<size_t, Reg_ptr>> &todas) const {
    todas.insert(todas.end(), entradas.begin(), entradas.end());
    if (overflowBucket) {
      overflowBucket->obtenerTodasEntradas(todas);
    }
  }

  // Obtiene la profundidad local
  size_t obtenerProfundidad() const {
    return localDepth;
  }

  // Establece la profundidad local
  void establecerProfundidad(size_t depth) {
    localDepth = depth;
    if (overflowBucket) {
      overflowBucket->establecerProfundidad(depth);
    }
  }

  // Verifica si este bucket (sin contar overflows) está lleno
  bool estaLleno(size_t tamBucket) const {
    return entradas.size() >= tamBucket;
  }

  // Verifica si este bucket y todos sus overflows están llenos
  bool todosLlenos(size_t tamBucket) const {
    if (entradas.size() < tamBucket)
      return false;
    if (!overflowBucket)
      return true;
    return overflowBucket->todosLlenos(tamBucket);
  }

  // Cuenta el número total de entradas (incluyendo overflows)
  size_t contarEntradas() const {
    size_t count = entradas.size();
    if (overflowBucket) {
      count += overflowBucket->contarEntradas();
    }
    return count;
  }

  // Cuenta el número de overflow buckets en esta cadena
  size_t contarOverflows() const {
    if (!overflowBucket)
      return 0;
    return 1 + overflowBucket->contarOverflows();
  }
};

// Tabla de Extendible Hashing con overflow buckets
class Hasher {
private:
  size_t globalDepth;
  size_t tamBucket;
  std::vector<std::shared_ptr<Bucket_>> directorio;

  // Función para calcular el hash de una clave
  size_t calcularHash(const std::string &clave) const {
    return std::hash<std::string>{}(clave);
  }

  // Obtiene el índice en el directorio
  size_t obtenerIndiceDirectorio(size_t hash) const {
    return hash & ((1 << globalDepth) - 1);
  }

  // Divide un bucket cuando es necesario
  void dividirBucket(std::shared_ptr<Bucket_> bucketViejo, size_t indice) {
    size_t profundidadLocal = bucketViejo->obtenerProfundidad();

    // Creamos un nuevo bucket
    auto bucketNuevo = std::make_shared<Bucket_>(profundidadLocal + 1);
    size_t mascara = 1 << profundidadLocal;

    // Obtenemos todas las entradas (incluyendo overflows)
    std::list<std::pair<size_t, Reg_ptr>> todasEntradas;
    bucketViejo->obtenerTodasEntradas(todasEntradas);

    // Limpiamos el bucket viejo y sus overflows
    bucketViejo->establecerProfundidad(profundidadLocal + 1);
    bucketViejo = std::make_shared<Bucket_>(profundidadLocal + 1);

    // Reinsertamos las entradas en los buckets correspondientes
    for (const auto &entrada : todasEntradas) {
      size_t newIdx = obtenerIndiceDirectorio(entrada.first);
      if ((indice & mascara) == (newIdx & mascara)) {
        bucketViejo->insertar(entrada.first, entrada.second, tamBucket);
      } else {
        bucketNuevo->insertar(entrada.first, entrada.second, tamBucket);
      }
    }

    // Actualizamos el directorio
    for (size_t i = 0; i < directorio.size(); ++i) {
      if (directorio[i] == bucketViejo) {
        if ((i & mascara) != (indice & mascara)) {
          directorio[i] = bucketNuevo;
        }
      }
    }
  }

  // Duplica el directorio cuando es necesario
  void duplicarDirectorio() {
    size_t tamViejo = directorio.size();
    directorio.resize(tamViejo * 2);

    for (size_t i = tamViejo; i < directorio.size(); ++i) {
      directorio[i] = directorio[i - tamViejo];
    }

    globalDepth++;
  }

public:
  size_t hashed_col_index{};

  Hasher(size_t tamBucket = 4, size_t col_index = 0) : globalDepth(0), tamBucket(tamBucket),
                                                       hashed_col_index(col_index) {
    directorio.push_back(std::make_shared<Bucket_>(0));
  }

  // Inserta un registro en la tabla hash
  void insertar(const std::string &clave, const Reg_ptr &regPtr) {
    size_t hash = calcularHash(clave);
    size_t indice = obtenerIndiceDirectorio(hash);
    auto bucket = directorio[indice];

    // Insertamos en el bucket (que manejará el overflow si es necesario)
    bucket->insertar(hash, regPtr, tamBucket);

    // Solo dividimos si todos los buckets en la cadena están llenos
    if (bucket->todosLlenos(tamBucket)) {
      if (bucket->obtenerProfundidad() == globalDepth) {
        duplicarDirectorio();
        indice = obtenerIndiceDirectorio(hash);
      }

      dividirBucket(bucket, indice);
    }
  }

  // Busca un registro por su clave
  std::vector<Reg_ptr> buscar(const std::string &clave) const {
    size_t hash = calcularHash(clave);
    size_t indice = obtenerIndiceDirectorio(hash);
    auto bucket = directorio[indice];

    std::vector<Reg_ptr> resultados;
    std::list<std::pair<size_t, Reg_ptr>> todasEntradas;
    bucket->obtenerTodasEntradas(todasEntradas);

    for (const auto &entrada : todasEntradas) {
      if (entrada.first == hash) {
        resultados.push_back(entrada.second);
      }
    }

    return resultados;
  }

  // Elimina un registro
  bool eliminar(const std::string &clave, const Reg_ptr &regPtr) {
    size_t hash = calcularHash(clave);
    size_t indice = obtenerIndiceDirectorio(hash);
    auto bucket = directorio[indice];

    return bucket->eliminar(regPtr);
  }

  // Obtiene estadísticas de la tabla hash
  void obtenerEstadisticas() const {
    std::cout << "Profundidad global: " << globalDepth << std::endl;
    std::cout << "Tamaño del directorio: " << directorio.size() << std::endl;

    std::unordered_map<std::shared_ptr<Bucket_>, size_t> bucketsUnicos;
    for (const auto &bucketPtr : directorio) {
      bucketsUnicos[bucketPtr]++;
    }

    std::cout << "Número de buckets únicos: " << bucketsUnicos.size() << std::endl;

    for (const auto &pair : bucketsUnicos) {
      std::cout << "  Bucket con profundidad " << pair.first->obtenerProfundidad()
                << ", entradas principales: " << pair.first->contarEntradas()
                << ", overflows: " << pair.first->contarOverflows()
                << ", apuntado por " << pair.second << " índices" << std::endl;
    }
  }

  // Método para inicializar el hash con los registros existentes
  template <typename Iter>
  void inicializar(const std::string &columna, Iter begin, Iter end) {
    for (auto it = begin; it != end; ++it) {
      std::string valorColumna = obtenerValorColumna(*it, columna);
      Reg_ptr ptr(it->pagina, it->slot);
      insertar(valorColumna, ptr);
    }
  }

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
      std::string valor = SQL_type_to_string(reg.values[col_idx]);
      std::println("{}", valor);

      // Insertar en el índice
      this->insertar(valor, Reg_ptr(reg.page_id, reg.position));
    }
  }

  // Método para actualizar el índice con un ResultSet de operaciones de inserción
  void actualizarDesdeInsercion(const ResultSet &resultSet, const std::string &columna) {
    indexarResultSet(resultSet, columna);
  }

  // Método para actualizar el índice con un ResultSet de operaciones de eliminación
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
      this->eliminar(valor, Reg_ptr(reg.page_id, reg.position));
    }
  }

  // Método para buscar y devolver un ResultSet
  std::vector<Reg_ptr> buscarResultSet(const std::string &clave) {
    return this->buscar(clave);
  }

private:
  // Función auxiliar para obtener el valor de una columna
  std::string obtenerValorColumna(const auto &registro, const std::string &columna) {
    // Implementación específica de tu SGBD
    return "";
  }
};
