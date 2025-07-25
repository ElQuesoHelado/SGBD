#include "bptree/bpnode.hpp"
#include "bptree/bptree.hpp"
#include "disk_manager.hpp"
#include "hash/directory.hpp"
#include "hash/hasher.hpp"
#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include "serial/sector0.hpp"
#include "serial/sector1.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <print>
#include <string>

void Megatron::translate_hash_directory(
    serial::TableMetadata &table_metadata, DirectoryPage &dir) {

  std::string out_str =
      "DIRECTORIO HASH de: " +
      std::string(array_to_string_view(
          table_metadata.name)) +
      '\n' +
      "Pagina: " + std::to_string(dir.page_id) +
      " Global_Depth: " + std::to_string((dir.global_depth)) +
      " Capacidad: " + std::to_string(dir.capacity) + "\n";

  // Primer sector tiene la metadata de pagina
  std::vector<std::string> sectors;
  sectors.emplace_back();

  int remm_sector_bytes =
      disk_manager->SECTOR_SIZE -
      sizeof(DirectoryPage::global_depth) -
      sizeof(DirectoryPage::capacity);

  sectors.back() += out_str + "\nPunteros a buckets:\n";

  size_t ith_sector_in_block{};
  sectors.back() +=
      disk_manager->logic_sector_to_CHS(
          disk_manager->free_block_map.get_ith_lba(
              dir.page_id, ith_sector_in_block)) +
      "\n";

  for (auto b : dir.bucket_ptrs) {
    if (b == disk_manager->NULL_BLOCK)
      continue;

    sectors.back() +=
        std::to_string(b);

    sectors.back() += '\n';

    remm_sector_bytes -= sizeof(uint32_t);

    if (remm_sector_bytes <= 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;
      ith_sector_in_block++;

      // std::println("{}", ith_sector_in_block);
      // if (ith_sector_in_block >= disk_manager->SECTORS_PER_BLOCK)
      //   continue;

      sectors.emplace_back();
      sectors.back() +=
          disk_manager->logic_sector_to_CHS(
              disk_manager->free_block_map.get_ith_lba(
                  dir.page_id, ith_sector_in_block)) +
          "\n" + "\nPunteros a buckets:\n";
    }
  }

  size_t i{};
  std::string page_str{};
  for (auto &e : sectors) {
    page_str += e;
    disk_manager->write_sector_txt(
        e,
        disk_manager->free_block_map.get_ith_lba(dir.page_id, i));
    // disk_manager->write_block_txt(e, page_id);
    i++;
  }

  disk_manager->write_block_txt(page_str, dir.page_id);
}

void Megatron::translate_bucket(
    serial::TableMetadata &table_metadata, Bucket &bucket,
    Hasher &hasher,
    std::vector<unsigned char> &page_bytes,
    uint32_t curr_page_id, uint16_t capacity) {

  // BPNode node(page_bytes, tree.key_type,
  //             tree.key_size, curr_page_id,
  //             tree.min_degree);

  std::string out_str =
      "BUCKET hash de: " +
      std::string(array_to_string_view(
          table_metadata.name)) +
      '\n' +
      "Pagina: " + std::to_string(bucket.page_id) +
      " Local_depth: " + std::to_string((bucket.local_depth)) +
      " Capacidad: " + std::to_string((bucket.capacity)) +
      " Size: " + std::to_string(bucket.size) +
      "\nPrefijo: " + std::to_string((bucket.prefix));

  // Primer sector tiene la metadata de pagina
  std::vector<std::string> sectors;
  sectors.emplace_back();

  int remm_sector_bytes =
      disk_manager->SECTOR_SIZE - sizeof(Bucket::overflow_page) -
      sizeof(Bucket::local_depth) - sizeof(Bucket::size) -
      sizeof(Bucket::capacity);

  size_t ith_sector_in_block{};

  sectors.back() += out_str +
                    disk_manager->logic_sector_to_CHS(
                        disk_manager->free_block_map.get_ith_lba(
                            bucket.page_id, ith_sector_in_block)) +
                    "\n";

  for (size_t i{}; i < bucket.capacity; ++i) {
    if (bucket.reg_ptrs[i].page_id != disk_manager->NULL_BLOCK &&
        bucket.reg_ptrs[i].page_id != 0) {
      sectors.back() +=
          SQL_type_to_string(bucket.keys[i]) + " " +
          std::to_string(bucket.reg_ptrs[i].page_id) +
          std::to_string(bucket.reg_ptrs[i].slot);
    } else {
      sectors.back() += "VACIO";
    }

    sectors.back() += '\n';

    remm_sector_bytes -= hasher.key_size;
    remm_sector_bytes -= sizeof(uint16_t);
    remm_sector_bytes -= sizeof(uint32_t);

    if (remm_sector_bytes <= 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;
      ith_sector_in_block++;

      if (ith_sector_in_block >= disk_manager->SECTORS_PER_BLOCK)
        continue;

      sectors.emplace_back();
      if (curr_page_id == disk_manager->NULL_BLOCK || curr_page_id == 0) {
        sectors.back() += "VACIO\n";

      } else {
        sectors.back() += disk_manager->logic_sector_to_CHS(
                              disk_manager->free_block_map.get_ith_lba(
                                  curr_page_id, ith_sector_in_block)) +
                          "\n" + "\nPunteros a registros:\n";
      }
    }
  }

  size_t i{};
  std::string page_str{};
  for (auto &e : sectors) {
    page_str += e;
    disk_manager->write_sector_txt(
        e,
        disk_manager->free_block_map.get_ith_lba(bucket.page_id, i));
    // disk_manager->write_block_txt(e, page_id);
    i++;
  }

  disk_manager->write_block_txt(page_str, bucket.page_id);
}
