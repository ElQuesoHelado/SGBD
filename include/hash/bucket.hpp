#pragma once
#include "serial/generic.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

#pragma pack(push, 1)
struct RegPtr_ {
  uint32_t page_id;
  uint32_t hash; // Valor(numero) de 8 bytes mas significaticos
  uint16_t slot;
};
#pragma pack(pop)

// Un bucket guarda sus propios datos
struct Bucket_ {
  uint32_t overflow_page;
  uint8_t reg_ptr_count;
  uint8_t local_depth;
  uint16_t max_reg_ptr_count; // Dicta cuando se llena
  std::vector<RegPtr_> reg_ptrs;
};

// Por bucket_size y tamanio fijo de RegPtr,
// podemos deserializar directamente los buckets
struct MultiBucketPage {
  uint32_t next_page;
  uint16_t bucket_count;
  uint16_t bucket_size; // Dicta cuando se llena
  std::vector<Bucket_> buckets;
};

template <typename Iter>
inline void serialize_bucket(const Bucket_ &bucket, Iter &out_it) {
  write_v(out_it, bucket.overflow_page);
  write_v(out_it, bucket.reg_ptr_count); // Garantiza que nunca sea != de n_slots
  write_v(out_it, bucket.local_depth);
  write_v(out_it, bucket.max_reg_ptr_count);

  for (auto &r : bucket.reg_ptrs)
    write_v(out_it, r);
}

template <typename Iter>
inline void serialize_multi_bucket_page(
    const MultiBucketPage &mb_page, Iter &out_it) {
  write_v(out_it, mb_page.next_page);
  write_v(out_it, mb_page.bucket_count); // Garantiza que nunca sea != de n_slots
  write_v(out_it, mb_page.bucket_size);

  for (auto &b : mb_page.buckets)
    serialize_bucket(b, out_it);
}

template <typename Iter>
inline Bucket_ deserialize_bucket(Iter &in_it) {
  Bucket_ bucket;

  read_v(in_it, bucket.overflow_page);
  read_v(in_it, bucket.reg_ptr_count);
  read_v(in_it, bucket.local_depth);
  read_v(in_it, bucket.max_reg_ptr_count);

  bucket.reg_ptrs.resize(bucket.reg_ptr_count);
  for (auto &r : bucket.reg_ptrs)
    read_v(in_it, r);

  return bucket;
}

inline MultiBucketPage deserialize_multi_bucket_page(
    std::vector<unsigned char> &bytes) {
  MultiBucketPage mb_page;
  auto it = bytes.begin();

  read_v(it, mb_page.next_page);
  read_v(it, mb_page.bucket_count);
  read_v(it, mb_page.bucket_size);

  mb_page.buckets.resize(mb_page.bucket_count);
  for (auto &b : mb_page.buckets) {
    b = deserialize_bucket(it);
  }

  return mb_page;
}
