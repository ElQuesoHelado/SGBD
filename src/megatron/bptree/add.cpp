// #include "hlpr.hpp"
#include "bptree/bptree.hpp"
#include "megatron.hpp"
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>

void Megatron::add_index_to_table(std::string &table_name, std::string &col_name) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return;
  }

  return add_index_to_table(table_metadata, col_name);
}
void Megatron::add_index_to_table(serial::TableMetadata &table_metadata, std::string &col_name) {
  auto col_index = get_column_index(table_metadata, col_name);

  // No existe columna
  if (col_index == table_metadata.n_cols) {
    std::println("Columna no existente para agregar hash");
    return;
  }

  return add_index_to_table(table_metadata, col_index);
}

void Megatron::add_index_to_table(serial::TableMetadata &table_metadata,
                                  size_t col_index) {
  // Busca si index ya existe
  if (is_column_indexed(table_metadata, col_index)) {
    std::println("Columna ya tiene un index asignado");
    return;
  }

  // size_t min_degree =
  //     calculate_btree_order(table_metadata.columns[col_index].max_size);

  size_t min_degree =
      2;

  BPTree tree(*buffer_manager, table_metadata,
              disk_manager->NULL_BLOCK,
              min_degree,
              table_metadata.columns[col_index].type,
              table_metadata.columns[col_index].max_size);

  std::vector<std::string> values = {
      std::to_string(table_metadata.table_block_id),
      std::to_string(col_index),
      "1",
      std::to_string(tree.root_id)};

  insert(catalog_name, values);

  Comparator comp;
  auto registers = select(table_metadata, comp);

  for (auto &reg : registers) {
    tree.insert(
        reg.values[col_index],
        {static_cast<uint32_t>(reg.page_id),
         static_cast<uint16_t>(reg.position)});
  }
}
