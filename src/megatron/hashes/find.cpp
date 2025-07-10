#include "hash/bucket.hpp"
#include "hash/directory.hpp"
#include "hlpr.hpp"
#include "result_set.hpp"
#include "serial/fixed_data.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <cstddef>
#include <iostream>

ResultSet Megatron::find(serial::TableMetadata table_metadata,
                         size_t hashed_col, SQL_type &key) {
  if (!is_column_hashed(table_metadata, hashed_col)) {
    std::cerr << "No se tiene hash para esa columna" << std::endl;
    return {};
  }

  auto dir_page = get_root_page_id(table_metadata, hashed_col);

  DirectoryPage dir = load_directory_page(dir_page);

  std::vector<unsigned char> key_bytes = serialize_sql_type(key);

  // En base a hash se ubica Bucket
  uint32_t bucket_ptr_index = hash_index(key_bytes,
                                         dir.global_depth);

  BucketPtr &bucket_ptr = dir.bucket_ptrs[bucket_ptr_index];

  ResultSet result_set{};
  result_set.add_columns(table_metadata.columns);

  size_t mb_page_id = bucket_ptr.mb_page;
  while (true) {
    auto &frame = buffer_manager->load_pin_page(mb_page_id);
    std::vector<unsigned char> &page_bytes = frame.page_bytes;

    MultiBucketPage mb_page = deserialize_multi_bucket_page(page_bytes);
    Bucket &bucket = mb_page.buckets[bucket_ptr.bucket_idx];

    // Se revisa todo registro en bucket(caso keys duplicadas)
    for (uint8_t i = 0; i < bucket.reg_ptr_count; i++) {
      auto partial_result_set =
          get_register_on_key_match(table_metadata,
                                    bucket.reg_ptrs[i],
                                    hashed_col,
                                    key);

      result_set.merge(std::move(partial_result_set));
    }

    // Se continua busqueda en overflow page
    if (bucket.overflow_page == disk_manager->NULL_BLOCK)
      break;

    buffer_manager->free_unpin_page(bucket_ptr.mb_page, 0);

    mb_page_id = bucket.overflow_page;
  }

  buffer_manager->free_unpin_page(bucket_ptr.mb_page, 0);
  return result_set;
}

// Caso el registro tenga key en la columna hashed, se retorna
ResultSet Megatron::get_register_on_key_match(
    serial::TableMetadata &table_metadata,
    RegPtr &reg_ptr, size_t hashed_col, SQL_type &key) {
  auto frame = buffer_manager->load_pin_page(reg_ptr.page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;

  auto page_bytes_it = page_bytes.begin();

  // Se saca metadata relevante
  auto page_header =
      serial::deserialize_page_header(page_bytes_it);

  serial::FixedDataHeader fixed_data_header;
  serial::SlottedDataHeader slotted_data_header;

  if (table_metadata.are_regs_fixed) {
    fixed_data_header =
        serial::deserialize_fixed_data_header(page_bytes_it);
    if (!fixed_data_header.free_register_bitmap.at(reg_ptr.slot)) {
      buffer_manager->free_unpin_page(reg_ptr.page_id, 0);
      return {};
    }

  } else {
    slotted_data_header =
        serial::deserialize_slotted_data_header(page_bytes_it);
    if (!slotted_data_header.slots[reg_ptr.slot].is_used) {
      buffer_manager->free_unpin_page(reg_ptr.page_id, 0);
      return {};
    }
  }

  std::vector<unsigned char> register_bytes;

  if (table_metadata.are_regs_fixed)
    register_bytes = get_ith_register_bytes(table_metadata, page_header,
                                            fixed_data_header,
                                            page_bytes, reg_ptr.slot);
  else
    register_bytes = get_ith_register_bytes(table_metadata, page_header,
                                            slotted_data_header,
                                            page_bytes, reg_ptr.slot);

  auto register_values =
      deserialize_register(table_metadata, register_bytes);

  ResultSet result_set;
  if (key == register_values[hashed_col]) {
    RegisterEntry reg{reg_ptr.page_id, reg_ptr.slot};

    for (auto &v : register_values)
      reg.values.push_back(v);

    result_set.add_register(std::move(reg));
  }

  buffer_manager->free_unpin_page(reg_ptr.page_id, 0);

  return result_set;
}
