#include "hlpr.hpp"
#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>

// void Megatron::insert_hashed(
//     serial::TableMetadata &table_metadata,
//     size_t col_index, size_t page_id, size_t pos,
//     std::vector<unsigned char> &register_bytes) {
//   auto dir_page_id = get_root_page_id(table_metadata, col_index);
//
//   if (dir_page_id == disk_manager->NULL_BLOCK) {
//     std::cerr << "No se tiene un hash para esa columna" << std::endl;
//     return;
//   }
//
//   // Se encuentra indice de bucket
//   auto depth = get_depth(table_metadata, col_index);
//   auto dir_index = hash_index(register_bytes, depth);
//
//   while (dir_page_id != disk_manager->NULL_BLOCK) {
//     auto &frame = buffer_manager->load_pin_page(page_id);
//     std::vector<unsigned char> &page_bytes = frame.page_bytes;
//     auto it = page_bytes.begin();
//
//     auto page_header = serial::deserialize_page_header(it);
//     auto fixed_data_header = serial::deserialize_fixed_data_header(it);
//
//     // FIXME: Parecido a update nth
//
//     dir_page_id = page_header.next_block_id;
//
//     buffer_manager->free_unpin_page(page_id, false);
//   }
// }

// bool Megatron::insert_hashed(
//     serial::TableMetadata table_metadata, size_t hashed_col,
//     SQL_type &key, size_t inserted_page, size_t inserted_slot) {
//   // if (!is_column_hashed(table_metadata, hashed_col)) {
//   //   std::cerr << "No se tiene hash para esa columna" << std::endl;
//   //   return {};
//   // }
//
//   auto dir_page = get_root_page_id(table_metadata, hashed_col);
//
//   DirectoryPage dir = load_directory_page(dir_page);
//
//   std::vector<unsigned char> key_bytes = serialize_sql_type(key);
//
//   uint32_t bucket_ptr_index = hash_index(key_bytes,
//                                          dir.global_depth);
//
//   BucketPtr &bucket_ptr = dir.bucket_ptrs[bucket_ptr_index];
//
//   auto &frame = buffer_manager->load_pin_page(bucket_ptr.mb_page);
//   std::vector<unsigned char> &page_bytes = frame.page_bytes;
//
//   MultiBucketPage mb_page = deserialize_multi_bucket_page(page_bytes);
//   Bucket &bucket = mb_page.buckets[bucket_ptr.bucket_idx];
//
//   // Intentar inserción directa
//   if (bucket.reg_ptr_count < bucket.max_reg_ptr_count) {
//     auto pos = find_free_reg_ptr_pos(bucket);
//     bucket.reg_ptrs[pos] = {static_cast<uint32_t>(inserted_page),
//                             static_cast<uint16_t>(inserted_slot)};
//
//     // bucket.reg_ptrs.emplace_back(inserted_page, inserted_slot);
//     bucket.reg_ptr_count++;
//
//     buffer_manager->free_unpin_page(bucket_ptr.mb_page, true);
//
//     return true;
//   }
//
//   // Bucket lleno, necesitamos split o overflow
//   if (bucket.local_depth < dir.global_depth) {
//     split_bucket(dir, entry, mb_page, bucket, bm);
//     bm->release_page(mb_page);
//     return insert(dir, hash, rid, bm); // Reintentar después del split
//   } else {
//     // Crear overflow
//     uint32_t overflow_page_id = bm->alloc_page();
//     MultiBucketPage *overflow_page = (MultiBucketPage *)bm->get_page(overflow_page_id);
//     overflow_page->bucket_count = 1;
//     overflow_page->free_offset = MAX_RIDS_PER_BUCKET * RID_SIZE;
//     overflow_page->buckets[0] = {0, 1, bucket->local_depth, 0};
//     *((RID *)overflow_page->data) = rid;
//
//     bucket->overflow_page = overflow_page_id;
//     bm->release_page(overflow_page);
//     bm->release_page(mb_page);
//     return true;
//   }
// }

// void redistribute_rids(DirectoryPage *dir, uint32_t old_mb_page, uint8_t old_bucket_idx,
//                        uint32_t new_mb_page, uint8_t new_bucket_idx, BufferManager *bm) {
//   MultiBucketPage *old_page = (MultiBucketPage *)bm->get_page(old_mb_page);
//   MultiBucketPage *new_page = (MultiBucketPage *)bm->get_page(new_mb_page);
//
//   Bucket *old_bucket = &old_page->buckets[old_bucket_idx];
//   Bucket *new_bucket = &new_page->buckets[new_bucket_idx];
//
//   RID *old_rids = (RID *)(old_page->data + old_bucket->offset);
//   RID *new_rids = (RID *)(new_page->data + new_bucket->offset);
//
//   uint8_t old_pos = 0;
//   uint32_t mask = 1 << (old_bucket->local_depth - 1);
//
//   // Redistribuir RIDs existentes
//   for (uint8_t i = 0; i < old_bucket->reg_ptr_count; i++) {
//     uint32_t rid_hash = hash_rid(old_rids[i]); // hash_rid() debe ser implementado
//     if (rid_hash & mask) {
//       // Mover al nuevo bucket
//       new_rids[new_bucket->reg_ptr_count++] = old_rids[i];
//     } else {
//       // Quedarse en el bucket viejo
//       old_rids[old_pos++] = old_rids[i];
//     }
//   }
//
//   old_bucket->reg_ptr_count = old_pos;
//   old_bucket->local_depth++;
//
//   bm->release_page(old_page);
//   bm->release_page(new_page);
// }
