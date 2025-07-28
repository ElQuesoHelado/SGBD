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
#include <string>

/*
 * Se traduce todo dato contenido en disco
 */
std::pair<std::vector<uint32_t>, std::vector<uint32_t>> Megatron::translate() {
  buffer_manager->flush_all();
  // Se traduce sector0
  std::vector<unsigned char> buffer;
  disk_manager->read_sector(buffer, 0);
  auto sector0 = serial::deserialize_sector0(buffer);

  DiskManager::create_disk_structure(
      0, disk_manager->disk_name,
      sector0.surfaces, sector0.tracks_per_surf,
      sector0.sectors_per_track, sector0.sector_size);

  auto format_str =
      std::format("Superficies: {}, tracks: {}, sectores: {}, tamanio de sector: {},"
                  "sectores por bloque: {}  \n",
                  sector0.surfaces, sector0.tracks_per_surf, sector0.sectors_per_track,
                  sector0.sector_size, sector0.sectors_per_block);

  disk_manager->write_sector_txt(format_str, 0);

  // Sector1
  disk_manager->read_sector(buffer, 1);
  auto sector1 = serial::deserialize_sector1(buffer);

  format_str = "Numero total de tablas: " + std::to_string(sector1.n_tables) + "\n";

  for (size_t i{}; i < sector1.n_tables; ++i)
    format_str += "Tabla #" + std::to_string(i) +
                  " ubicada en bloque: " + std::to_string(sector1.table_block_ids[i]) + "\n";

  disk_manager->write_sector_txt(format_str, 1);

  disk_manager->clear_blocks_folder();

  std::vector<uint32_t> hashed_pages{}, indexed_pages{};

  // Todas las tablas
  for (size_t i{}; i < sector1.n_tables; ++i) {
    std::vector<unsigned char> table_block;
    disk_manager->read_block(table_block, sector1.table_block_ids[i]);
    auto table_metadata = serial::deserialize_table_metadata(table_block);

    // Translate de metadata de tabla
    disk_manager->write_block_txt(translate_table_page(table_metadata), table_metadata.table_block_id);

    // Se itera por toda pagina de tabla, se traduce a bloques y sectores
    uint32_t curr_page_id = table_metadata.first_page_id;

    while (curr_page_id != disk_manager->NULL_BLOCK) {
      std::vector<unsigned char> page_bytes;
      disk_manager->read_block(page_bytes, curr_page_id);
      std::span<unsigned char> page_data(page_bytes);

      serial::PageHeader page_header(page_data);

      auto page_str = translate_data_page(table_metadata, page_bytes, curr_page_id);

      disk_manager->write_block_txt(page_str, curr_page_id);

      curr_page_id = page_header.next_block_id;
      // std::cout << curr_page_id << std::endl;
    }

    // Translate de indices/hashes
    auto hashed_cols = get_hashed_columns(table_metadata);
    for (auto &[c, r] : hashed_cols) {
      Hasher hasher(
          *buffer_manager,
          r,
          table_metadata.columns[c].type,
          table_metadata.columns[c].max_size);

      std::vector<unsigned char> dir_page_bytes;
      disk_manager->read_block(dir_page_bytes, r);

      DirectoryPage dir(dir_page_bytes, r, disk_manager->NULL_BLOCK,
                        hasher.directory_capacity);

      translate_hash_directory(table_metadata, dir);

      hashed_pages.push_back(dir.page_id);

      for (auto b_page_id : dir.bucket_ptrs) {
        if (b_page_id == disk_manager->NULL_BLOCK)
          continue;

        std::vector<unsigned char> bucket_page_bytes;
        disk_manager->read_block(bucket_page_bytes, b_page_id);
        Bucket bucket(bucket_page_bytes, b_page_id,
                      hasher.bucket_capacity, hasher.key_type,
                      hasher.key_size, disk_manager->NULL_BLOCK);

        hashed_pages.push_back(b_page_id);

        translate_bucket(table_metadata, bucket, hasher,
                         bucket_page_bytes, b_page_id,
                         hasher.bucket_capacity);
      }
    }

    auto indexed_cols = get_indexed_columns(table_metadata);
    for (auto &[c, r] : indexed_cols) {
      auto min_degree =
          calculate_btree_order(table_metadata.columns[c].max_size);
      BPTree tree(*buffer_manager, table_metadata,
                  r,
                  min_degree,
                  table_metadata.columns[c].type,
                  table_metadata.columns[c].max_size);

      indexed_pages = translate_bptree_node_page(table_metadata, tree, tree.root_id);
    }
  }

  return {hashed_pages, indexed_pages};
}

std::string Megatron::translate_table_page(serial::TableMetadata &table_metadata) {
  std::string out_str{};
  out_str += std::to_string(table_metadata.table_block_id) + "\n";
  out_str += std::string(array_to_string_view(table_metadata.name)) + '\n';
  out_str += "Max_reg_size: " + std::to_string(table_metadata.max_reg_size) + '\n';
  out_str += "Fixed_regs: " + std::to_string(table_metadata.are_regs_fixed) + '\n';
  out_str += "First page id: " + std::to_string(table_metadata.first_page_id) + '\n';
  out_str += "Last page id: " + std::to_string(table_metadata.last_page_id) + '\n';
  out_str += "n_columns " + std::to_string(table_metadata.n_cols) + '\n';

  for (auto &c : table_metadata.columns) {
    out_str += std::string(array_to_string_view(c.name)) + " ";
    out_str += std::to_string(c.type) + " ";
    out_str += std::to_string(c.max_size) + " ";
    out_str += "\n";
  }

  return out_str;
}
