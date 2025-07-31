#include "megatron.hpp"
#include "serial/fixed_page.hpp"
#include "serial/slotted_page.hpp"
#include "types/types.hpp"
#include <iostream>
#include <print>

#ifdef __GNUC__
template <typename T1, typename T2>
struct std::formatter<std::pair<T1, T2>> : std::formatter<std::string> {
    auto format(const std::pair<T1, T2>& p, std::format_context& ctx) const {
      return std::format_to(ctx.out(), "({}:{})", p.first, p.second);
  }
};
#endif

void Megatron::select_print(std::string &table_name,
                            Comparator &comparator,
                            int max_pages_loaded) {
  auto result_set = select(table_name, comparator, max_pages_loaded);

  size_t i{1};
  for (auto &reg : result_set) {
    std::println("Bloque: {} ({}) {}", reg.reg_ptr.page_id, i, reg);
    i++;
  }
}

void Megatron::select_print(serial::TableMetadata &table_metadata,
                            Comparator &comparator,
                            int max_pages_loaded) {
  auto result_set = select(table_metadata, comparator, max_pages_loaded);

  size_t i{1};
  for (auto &reg : result_set) {
    std::println("Bloque: {} ({}) {}", reg.reg_ptr.page_id, i, reg);
    i++;
  }
}

ResultSet Megatron::select(std::string &table_name,
                           Comparator &comparator,
                           int max_pages_loaded) {
  serial::TableMetadata table_metadata;

  // No existe
  if (!search_table(table_name, table_metadata)) {
    std::cerr << "Tabla: " << table_name << " no existe" << std::endl;
    return {};
  }

  return select(table_metadata, comparator, max_pages_loaded);
}

ResultSet Megatron::select(serial::TableMetadata &table_metadata,
                           Comparator &comparator,
                           int max_pages_loaded) {
  ResultSet result_set{};
  result_set.add_columns(table_metadata.columns);

  // Se puede usar hash para busqueda
  if (comparator.is_only_equals() &&
      is_column_hashed(table_metadata,
                       comparator.col_index_at_op(0))) {
    auto hashed_col_index = comparator.col_index_at_op(0);
    auto dir_page =
        get_root_page_id(table_metadata, hashed_col_index);

    std::println("Se tiene hash en columna #: {}/{} y solo condicion "
                 "igualdad, se usa hash para busqueda",
                 hashed_col_index, result_set.columns[hashed_col_index]);

    std::println("Pagina directorio en: {}", dir_page);
    Hasher hasher(*buffer_manager,
                  dir_page,
                  table_metadata.columns[hashed_col_index].type,
                  table_metadata.columns[hashed_col_index].max_size);

    auto reg_ptrs =
        hasher.search(comparator);

    for (auto &r : reg_ptrs) {
      result_set.merge(std::move(
          select_nth_from_page(table_metadata, r.page_id,
                               r.slot)));
    }

    return result_set;
  } else if (comparator.is_ranged_on_single_col() &&
             is_column_indexed(table_metadata,
                               comparator.col_index_at_op(0))) {

    auto ic =
        get_indexed_column(table_metadata, comparator.col_index_at_op(0));
    auto col_index = ic.first;
    auto root_id = ic.second;
    auto min_degree =
        calculate_btree_order(table_metadata.columns[col_index].max_size);

    std::println("Se tiene indice en columna #: {}/{} y condicion "
                 "por rango, se usa b+tree para busqueda",
                 ic, result_set.columns[col_index]);

    BPTree tree(*buffer_manager, table_metadata,
                root_id,
                min_degree,
                table_metadata.columns[col_index].type,
                table_metadata.columns[col_index].max_size);

    auto reg_ptrs = tree.search(comparator);

    for (auto &r : reg_ptrs) {
      result_set.merge(std::move(
          select_nth_from_page(table_metadata, r.page_id,
                               r.slot)));
    }
    return result_set;
  }

  // Busqueda secuencial
  size_t curr_page_id = table_metadata.first_page_id;
  while (curr_page_id != disk_manager->NULL_BLOCK && max_pages_loaded != 0) {
    auto frame = buffer_manager->load_pin_page(curr_page_id);
    max_pages_loaded--;

    std::span<unsigned char> page_data(frame.page_bytes);

    serial::PageHeader page_header(page_data);

    // auto page_header = serial::deserialize_page_header(page_bytes);

    buffer_manager->free_unpin_page(curr_page_id, false);

    auto partial_result_set =
        select_from_page(table_metadata, curr_page_id, comparator);

    result_set.merge(std::move(partial_result_set));

    curr_page_id = page_header.next_block_id;
  }

  return result_set;
}

ResultSet Megatron::select_from_page(serial::TableMetadata &table_metadata,
                                     uint32_t select_page_id, Comparator &comparator) {
  auto result_set =
      (table_metadata.are_regs_fixed)
          ? select_from_fixed_page(table_metadata, select_page_id, comparator)
          : select_from_slotted_page(table_metadata, select_page_id, comparator);

  return result_set;
}

ResultSet Megatron::select_from_fixed_page(
    serial::TableMetadata &table_metadata,
    uint32_t select_page_id, Comparator &comparator) {
  auto frame = buffer_manager->load_pin_page(select_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;
  std::span<unsigned char> page_data(page_bytes);

  serial::FixedPage fixed_page(page_data, page_bytes);

  // Se saca metadata relevante
  auto &page_header = fixed_page.page_header;
  auto &fixed_data_header = fixed_page.fixed_data_header;

  ResultSet result_set;
  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    if (fixed_data_header.free_register_bitmap->test(i)) { // Registro existe
      auto register_bytes = get_ith_register_bytes(table_metadata, page_header, fixed_data_header, page_bytes, i);
      auto register_values = deserialize_register(table_metadata, register_bytes);

      // Si es que hay condicion
      if (!comparator.evaluate(register_values))
        continue;

      RegisterEntry reg(select_page_id,
                        i, std::move(register_values));

      result_set.add_register(std::move(reg));
    }
  }

  buffer_manager->free_unpin_page(select_page_id, 0);
  return result_set;
}

ResultSet Megatron::select_from_slotted_page(
    serial::TableMetadata &table_metadata,
    uint32_t select_page_id, Comparator &comparator) {
  auto frame = buffer_manager->load_pin_page(select_page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;
  std::span<unsigned char> page_data(page_bytes);

  auto page_bytes_it = page_bytes.begin();

  serial::SlottedPage slotted_page(page_data, page_bytes);

  // Se saca metadata relevante
  auto &page_header = slotted_page.page_header;
  auto &slotted_data_header = slotted_page.slotted_data_header;

  ResultSet result_set;
  for (size_t i{}; i < slotted_data_header.n_slots; ++i) {
    if (slotted_data_header.slots[i].is_used) { // Registro existe
      auto register_bytes = get_ith_register_bytes(table_metadata, page_header,
                                                   slotted_data_header, page_bytes, i);
      auto register_values = deserialize_register(table_metadata, register_bytes);

      // Si es que hay condicion
      if (!comparator.evaluate(register_values))
        continue;

      RegisterEntry reg(select_page_id,
                        i, std::move(register_values));

      result_set.add_register(std::move(reg));
    }
  }

  buffer_manager->free_unpin_page(select_page_id, 0);

  return result_set;
}
