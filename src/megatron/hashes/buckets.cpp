#include "hash/bucket.hpp"
#include "megatron.hpp"
#include <cstddef>

size_t Megatron::max_buckets_per_page(MultiBucketPage &mb_page) {
  // size_t bucket_size = (sizeof(bucket.overflow_page) +
  //                       sizeof(bucket.reg_ptr_count) +
  //                       sizeof(bucket.local_depth) +
  //                       sizeof(bucket.max_reg_ptr_count) +
  //                       bucket.max_reg_ptr_count * sizeof(RegPtr));

  size_t multi_bucket_page_size = sizeof(MultiBucketPage::next_page) +
                                  sizeof(MultiBucketPage::bucket_count) +
                                  sizeof(MultiBucketPage::bucket_size);

  return (disk_manager->BLOCK_SIZE - multi_bucket_page_size) /
         mb_page.bucket_size;
}

// void split_bucket(DirectoryPage *dir, DirEntry *entry,
//                   MultiBucketPage *mb_page, Bucket *bucket) {
//   // Incrementar profundidad local
//   uint8_t new_depth = bucket->local_depth + 1;
//
//   // Encontrar espacio para nuevo bucket
//   uint32_t new_mb_page_id;
//   uint8_t new_bucket_idx;
//   MultiBucketPage *new_mb_page = find_free_bucket_slot(dir, new_mb_page_id, new_bucket_idx, bm);
//
//   if (!new_mb_page) {
//     new_mb_page_id = bm->alloc_page();
//     new_mb_page = (MultiBucketPage *)bm->get_page(new_mb_page_id);
//     new_mb_page->bucket_count = 0;
//     new_mb_page->free_offset = 0;
//     new_bucket_idx = 0;
//   }
//
//   // Configurar nuevo bucket
//   new_mb_page->buckets[new_bucket_idx] = {
//       new_mb_page->free_offset,
//       0,
//       new_depth,
//       0};
//   new_mb_page->free_offset += MAX_RIDS_PER_BUCKET * RID_SIZE;
//   new_mb_page->bucket_count++;
//
//   // Actualizar directorio
//   uint32_t mask = 1 << bucket->local_depth;
//   for (uint32_t i = 0; i < (1 << dir->global_depth); i++) {
//     if (dir->entries[i].mb_page == entry->mb_page &&
//         dir->entries[i].bucket_idx == entry->bucket_idx) {
//       if (i & mask) {
//         dir->entries[i] = {new_mb_page_id, new_bucket_idx, new_depth};
//       } else {
//         dir->entries[i].local_depth = new_depth;
//       }
//     }
//   }
//
//   // Redistribuir RIDs
//   redistribute_rids(dir, entry->mb_page, entry->bucket_idx, new_mb_page_id, new_bucket_idx, bm);
//
//   bm->release_page(new_mb_page);
// }

size_t Megatron::find_free_reg_ptr_pos(Bucket_ &bucket) {
  for (size_t i{}; i < bucket.max_reg_ptr_count; ++i) {
    if (bucket.reg_ptrs[i].page_id == disk_manager->NULL_BLOCK)
      return i;
  }
  return bucket.max_reg_ptr_count;
}

// MultiBucketPage *find_free_bucket_slot(DirectoryPage *dir, uint32_t &found_page_id, uint8_t &found_idx, BufferManager *bm) {
//   // Busca en todas las páginas multibucket
//
//   for (uint32_t i = 0; i < dir->bucket_ptr_count; i++) {
//     MultiBucketPage *mb_page = (MultiBucketPage *)bm->get_page(dir->entries[i].mb_page);
//     if (mb_page->bucket_count < MAX_BUCKETS_PER_PAGE) {
//       // Encontrar primer bucket no usado
//       for (uint8_t j = 0; j < MAX_BUCKETS_PER_PAGE; j++) {
//         if (mb_page->buckets[j].rid_count == 0 && mb_page->buckets[j].local_depth == 0) {
//           found_page_id = dir->entries[i].mb_page;
//           found_idx = j;
//           return mb_page;
//         }
//       }
//     }
//     bm->release_page(mb_page);
//   }
//
//   return nullptr;
// }
