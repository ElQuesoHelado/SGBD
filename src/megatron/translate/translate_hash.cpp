#include "megatron.hpp"
#include <print>

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

  for (auto b : dir.bucket_ptrs) {
    sectors.back() +=
        std::to_string(b);

    sectors.back() += ' ';

    remm_sector_bytes -= sizeof(uint32_t);

    if (remm_sector_bytes <= 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;
      ith_sector_in_block++;

      // std::println("{}", ith_sector_in_block);
      // if (ith_sector_in_block >= disk_manager->SECTORS_PER_BLOCK)
      //   continue;
      // sectors.back() += '\n';

      sectors.emplace_back();
    }
  }

  while (sectors.size() > disk_manager->SECTORS_PER_BLOCK) {
    auto mod = sectors.size() % disk_manager->SECTORS_PER_BLOCK;
    auto sect_str = sectors.back();
    sectors.pop_back();

    sectors[mod] += sect_str;
  }

  sectors.resize(disk_manager->SECTORS_PER_BLOCK);

  size_t i{};
  std::string page_str{}, sect_str;
  for (auto &e : sectors) {
    sect_str = disk_manager->logic_sector_to_CHS(
                   disk_manager->free_block_map.get_ith_lba(
                       dir.page_id, i)) +
               '\n' +
               e;
    page_str += sect_str;
    disk_manager->write_sector_txt(
        sect_str,
        disk_manager->free_block_map
            .get_ith_lba(dir.page_id, i));
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

  std::string out_str =
      "BUCKET hash de: " +
      std::string(array_to_string_view(
          table_metadata.name)) +
      '\n' +
      "Pagina: " + std::to_string(bucket.page_id) +
      " Local_depth: " + std::to_string((bucket.local_depth)) +
      " Capacidad: " + std::to_string((bucket.capacity)) +
      " Size: " + std::to_string(bucket.size) +
      "\nPrefijo: " + std::to_string((bucket.prefix)) + "\n";

  // Primer sector tiene la metadata de pagina
  std::vector<std::string> sectors;
  sectors.emplace_back();
  sectors.back() += out_str;

  int remm_sector_bytes =
      disk_manager->SECTOR_SIZE -
      sizeof(Bucket::overflow_page) -
      sizeof(Bucket::prefix) -
      sizeof(Bucket::local_depth) -
      sizeof(Bucket::size) -
      sizeof(Bucket::capacity);

  int entry_size = (int)hasher.key_size - sizeof(uint16_t) - sizeof(uint32_t);

  for (size_t i{}; i < bucket.capacity; ++i) {
    if (remm_sector_bytes - entry_size < 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;

      sectors.emplace_back();
    }

    // if (bucket.reg_ptrs[i].page_id != disk_manager->NULL_BLOCK &&
    //     bucket.reg_ptrs[i].page_id != 0) { // FIXME: ?no se inicializa con nullptr?
    // } else {
    //   sectors.back() += "VACIO";
    // }
    sectors.back() +=
        SQL_type_to_string(bucket.keys[i]) + " (" +
        std::to_string(bucket.reg_ptrs[i].page_id) + ", " +
        std::to_string(bucket.reg_ptrs[i].slot) + ")";

    sectors.back() += '\n';

    remm_sector_bytes -= entry_size;
  }

  while (sectors.size() > disk_manager->SECTORS_PER_BLOCK) {
    auto mod = sectors.size() % disk_manager->SECTORS_PER_BLOCK;
    auto sect_str = sectors.back();
    sectors.pop_back();

    sectors[mod] += sect_str;
  }

  size_t i{};
  std::string page_str{}, sect_str;
  for (auto &e : sectors) {
    sect_str = disk_manager->logic_sector_to_CHS(
                   disk_manager->free_block_map.get_ith_lba(
                       bucket.page_id, i)) +
               '\n' + e;
    page_str += sect_str;
    disk_manager->write_sector_txt(
        sect_str,
        disk_manager->free_block_map
            .get_ith_lba(bucket.page_id, i));
    i++;
  }

  disk_manager->write_block_txt(page_str, bucket.page_id);
}
