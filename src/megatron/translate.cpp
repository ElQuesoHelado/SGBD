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
void Megatron::translate() {
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
                  " ubicada en bloque: " + std::to_string(sector1.table_block_ids[i]);

  disk_manager->write_sector_txt(format_str, 1);

  disk_manager->clear_blocks_folder();

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

      auto page_bytes_it = page_bytes.begin();

      auto page_header = serial::deserialize_page_header(page_bytes_it);

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

      auto dir =
          hasher.load_directory(hasher.directory_id);

      hasher.unload_directory(dir);

      translate_hash_directory(
          table_metadata, dir,
          std::vector<unsigned char> & page_bytes, uint32_t curr_page_id, hasher.capacity);

      for (auto b : dir.bucket_ptrs) {
      }
    }
  }
}

// Escribe sectores al disco
std::string Megatron::translate_data_page(serial::TableMetadata &table_metadata,
                                          std::vector<unsigned char> &page_bytes, size_t page_id) {
  auto page_bytes_it = page_bytes.begin();
  auto page_header = serial::deserialize_page_header(page_bytes_it);

  std::vector<std::string> sectors;
  if (table_metadata.are_regs_fixed) {
    serial::FixedDataHeader fixed_data_header;
    fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);

    sectors = translate_fixed_page(table_metadata, page_header,
                                   fixed_data_header, page_bytes, page_id);

  } else {
    serial::SlottedDataHeader slotted_data_header;
    slotted_data_header = serial::deserialize_slotted_data_header(page_bytes_it);

    sectors = translate_slotted_page(table_metadata, page_header,
                                     slotted_data_header, page_bytes, page_id);
  }

  // size_t i{};
  // std::string page_str{};
  // for (auto &e : sectors) {
  //   page_str += e;
  //   i++;
  // }

  size_t i{};
  std::string page_str{};
  for (auto &e : sectors) {
    page_str += e;
    disk_manager->write_sector_txt(
        e,
        disk_manager->free_block_map.get_ith_lba(page_id, i));
    // disk_manager->write_block_txt(e, page_id);
    i++;
  }

  return page_str;
}

std::string Megatron::translate_data_page_no_write(serial::TableMetadata &table_metadata,
                                                   std::vector<unsigned char> &page_bytes, size_t page_id) {
  auto page_bytes_it = page_bytes.begin();
  auto page_header = serial::deserialize_page_header(page_bytes_it);

  std::vector<std::string> sectors;
  if (table_metadata.are_regs_fixed) {
    serial::FixedDataHeader fixed_data_header;
    fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);

    sectors = translate_fixed_page(table_metadata, page_header,
                                   fixed_data_header, page_bytes, page_id);

  } else {
    serial::SlottedDataHeader slotted_data_header;
    slotted_data_header = serial::deserialize_slotted_data_header(page_bytes_it);

    sectors = translate_slotted_page(table_metadata, page_header,
                                     slotted_data_header, page_bytes, page_id);
  }
  size_t i{};
  std::string page_str{};
  for (auto &e : sectors) {
    page_str += e;
    // disk_manager->write_block_txt(e, page_id);
    i++;
  }

  return page_str;
}

std::string Megatron::translate_data_page(serial::TableMetadata &table_metadata, size_t page_id) {
  std::vector<unsigned char> page_bytes;
  disk_manager->read_block(page_bytes, page_id);

  auto page_bytes_it = page_bytes.begin();

  auto page_header = serial::deserialize_page_header(page_bytes_it);

  std::vector<std::string> sectors;
  if (table_metadata.are_regs_fixed) {
    serial::FixedDataHeader fixed_data_header;
    fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);

    sectors = translate_fixed_page(table_metadata, page_header,
                                   fixed_data_header, page_bytes, page_id);

  } else {
    serial::SlottedDataHeader slotted_data_header;
    slotted_data_header = serial::deserialize_slotted_data_header(page_bytes_it);

    sectors = translate_slotted_page(table_metadata, page_header,
                                     slotted_data_header, page_bytes, page_id);
  }

  size_t i{};
  std::string page_str{};
  for (auto &e : sectors) {
    page_str += e;
    i++;
  }

  //   size_t i{};
  // std::string page_str{};
  // for (auto &e : sectors) {
  //   disk_manager->write_sector_txt(
  //       e,
  //       disk_manager->free_block_map.get_ith_lba(curr_page_id, i));
  //   disk_manager->write_block_txt(e, curr_page_id);
  //   i++;
  // }
  return page_str;
}

std::string Megatron::translate_table_page(serial::TableMetadata &table_metadata) {
  std::string out_str{};
  out_str += std::to_string(table_metadata.table_block_id);
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

// Es una pagina completa, implica varios sectores, el primero es afectado por el header
std::vector<std::string> Megatron::translate_fixed_page(
    serial::TableMetadata &table_metadata,
    serial::PageHeader &page_header,
    serial::FixedDataHeader &fixed_data_header,
    std::vector<unsigned char> &page_bytes, uint32_t curr_page_id) {

  // Primer sector tiene la metadata de pagina
  std::vector<std::string> sectors;
  sectors.emplace_back();

  int remm_sector_bytes =
      disk_manager->SECTOR_SIZE -
      serial::calculate_fixed_data_header_size(fixed_data_header); // Deberia restarse size de header
  size_t ith_sector_in_block{};

  // PageHeader
  std::string out_str{};
  out_str += "Next_page_id: " + std::to_string(page_header.next_block_id) + " ";
  out_str += "N_registers: " + std::to_string(page_header.n_regs) + " ";
  out_str += "Free_space/capacity: " + std::to_string(page_header.free_space) +
             '/' + std::to_string(disk_manager->BLOCK_SIZE - serial::calculate_fixed_data_header_size(fixed_data_header)) +
             "\n";

  // FixedDataHeader
  out_str += "Register size: " + std::to_string(fixed_data_header.reg_size) + " ";
  out_str += "Max registers: " + std::to_string(fixed_data_header.max_n_regs) + " ";
  std::string bitmap_str;
  boost::to_string(fixed_data_header.free_register_bitmap, bitmap_str);
  out_str += "Free_register_bitmap: " + bitmap_str + "\n" +
             disk_manager->logic_sector_to_CHS(
                 disk_manager->free_block_map.get_ith_lba(
                     curr_page_id, ith_sector_in_block)) +
             "\n";

  sectors.back() += out_str;

  // disk_manager->write_block_txt(out_str, curr_page_id);
  // disk_manager->write_block_txt(
  //     disk_manager->logic_sector_to_CHS(
  //         disk_manager->free_block_map.get_ith_lba(
  //             curr_page_id, ith_sector_in_block)),
  //     curr_page_id);

  for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
    // sectors.back()= "";
    if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
      auto register_bytes = get_ith_register_bytes(table_metadata,
                                                   page_header,
                                                   fixed_data_header, page_bytes, i);
      auto register_values = deserialize_register(table_metadata, register_bytes);

      for (auto &v : register_values)
        sectors.back() += SQL_type_to_string(v) + " ";

      sectors.back() += '\n';
    } else { // Registro vacio/deleted
      sectors.back() += '\n';
    }

    remm_sector_bytes -= fixed_data_header.reg_size;
    if (remm_sector_bytes <= 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;
      ith_sector_in_block++;
      // disk_manager->write_block_txt("\n" + disk_manager->logic_sector_to_CHS(
      //                                          disk_manager->free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block)),
      //                               curr_page_id);
      sectors.emplace_back();
      sectors.back() += disk_manager->logic_sector_to_CHS(
                            disk_manager->free_block_map.get_ith_lba(
                                curr_page_id, ith_sector_in_block)) +
                        "\n";
    }

    // disk_manager->write_sector_txt(
    //     out_str,
    //     disk_manager->free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block));
    // disk_manager->write_block_txt(out_str, curr_page_id);
  }
  return sectors;
}

std::vector<std::string> Megatron::translate_slotted_page(
    serial::TableMetadata &table_metadata,
    serial::PageHeader &page_header,
    serial::SlottedDataHeader &slotted_data_header,
    std::vector<unsigned char> &page_bytes, uint32_t curr_page_id) {
  std::vector<std::string> sectors;
  sectors.emplace_back();

  int remm_sector_bytes = disk_manager->SECTOR_SIZE;
  size_t ith_sector_in_block{disk_manager->SECTORS_PER_BLOCK - 1};

  // PageHeader
  std::string out_str{};
  out_str += "Next_page_id: " + std::to_string(page_header.next_block_id) + " ";
  out_str += "N_registers: " + std::to_string(page_header.n_regs) + " ";
  out_str += "Free_space/capacity: " + std::to_string(page_header.free_space) +
             '/' + std::to_string(disk_manager->BLOCK_SIZE - serial::calculate_slotted_data_header_size(slotted_data_header)) +
             "\n";

  // SlottedDataHeader
  out_str += "free_space_offset: " + std::to_string(slotted_data_header.free_space_offset) + " ";
  out_str += "# Slots: " + std::to_string(slotted_data_header.n_slots) + "\n |";

  for (auto &S : slotted_data_header.slots) {
    out_str += std::to_string(S.is_used) + '|';
    out_str += std::to_string(S.reg_size) + '|';
    out_str += std::to_string(S.offset_reg_start) + "|\n|";
  }

  sectors.back() += out_str + disk_manager->logic_sector_to_CHS(
                                  disk_manager->free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block));

  // disk_manager->write_block_txt(out_str, curr_page_id);
  // disk_manager->write_block_txt(disk_manager->logic_sector_to_CHS(
  //                                   disk_manager->free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block)),
  //                               curr_page_id);

  for (size_t i{}; i < slotted_data_header.n_slots; ++i) {
    // out_str = "";
    size_t sub{};
    if (slotted_data_header.slots[i].is_used) { // Registro existe
      auto register_bytes = get_ith_register_bytes(table_metadata,
                                                   page_header,
                                                   slotted_data_header, page_bytes, i);
      auto register_values = deserialize_register(table_metadata, register_bytes);

      // slotted_data_header.slots[i].
      for (auto &v : register_values)
        // disk.write
        sectors.back() += SQL_type_to_string(v) + " ";

      sub = register_bytes.size();
      sectors.back() += '\n';
    } else { // Registro vacio/deleted
      sectors.back() += '\n';
    }
    // Permite incluir
    remm_sector_bytes -= slotted_data_header.slots[i].reg_size;
    if (remm_sector_bytes < 0) {
      if (ith_sector_in_block == 0) // ultimo sector tiene cabecera
        remm_sector_bytes =
            disk_manager->SECTOR_SIZE -
            serial::calculate_slotted_data_header_size(slotted_data_header);
      else
        remm_sector_bytes = disk_manager->SECTOR_SIZE;

      sectors.emplace_back(); // TODO: ?Front?
      sectors.back() += "\n" + disk_manager->logic_sector_to_CHS(
                                   disk_manager->free_block_map.get_ith_lba(
                                       curr_page_id, ith_sector_in_block));
      ith_sector_in_block--;
      // disk_manager->write_block_txt(
      //     "\n" + disk_manager->logic_sector_to_CHS(
      //                disk_manager->free_block_map.get_ith_lba(
      //                    curr_page_id, ith_sector_in_block)),
      //     curr_page_id);
    }

    // disk_manager->write_sector_txt(
    //     out_str,
    //     disk_manager->free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block));
    // disk_manager->write_block_txt(out_str, curr_page_id);
  }

  return sectors;
}

std::vector<std::string> Megatron::translate_bptree_node_page(
    serial::TableMetadata &table_metadata, BPTree &tree,
    std::vector<unsigned char> &page_bytes,
    uint32_t curr_page_id) {

  BPNode node(page_bytes, tree.key_type,
              tree.key_size, curr_page_id,
              tree.min_degree);

  std::string out_str =
      "NODO B+tree de: " +
      std::string(array_to_string_view(
          table_metadata.name)) +
      '\n' +
      "Pagina: " + std::to_string(node.node_id) +
      " Es hoja: " + std::to_string((node.is_leaf)) +
      " N_llaves: " + std::to_string(node.n_keys) + "\n";

  // Primer sector tiene la metadata de pagina
  std::vector<std::string> sectors;
  sectors.emplace_back();

  int remm_sector_bytes =
      disk_manager->SECTOR_SIZE - sizeof(BPNode::is_leaf) -
      sizeof(BPNode::n_keys);

  size_t ith_sector_in_block{};

  sectors.back() += out_str;

  for (size_t i{}; i < node.n_keys; ++i) {
    sectors.back() +=
        SQL_type_to_string(node.keys[i]) + " " +
        std::to_string(node.ptrs[i]);

    if (node.is_leaf)
      sectors.back() += " " +
                        std::to_string(node.reg_slots[i]);

    sectors.back() += '\n';

    remm_sector_bytes -= tree.key_size;
    remm_sector_bytes -= sizeof(uint16_t);
    remm_sector_bytes -= sizeof(uint32_t);

    if (remm_sector_bytes <= 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;
      ith_sector_in_block++;

      sectors.emplace_back();
      sectors.back() += disk_manager->logic_sector_to_CHS(
                            disk_manager->free_block_map.get_ith_lba(
                                curr_page_id, ith_sector_in_block)) +
                        "\n";
    }
  }
  return sectors;
}

std::vector<std::string> Megatron::translate_hash_directory(
    serial::TableMetadata &table_metadata, DirectoryPage &dir,
    std::vector<unsigned char> &page_bytes,
    uint32_t curr_page_id, uint32_t capacity) {
  auto directory =
      DirectoryPage(page_bytes,
                    curr_page_id,
                    disk_manager->NULL_BLOCK,
                    capacity);

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

  size_t ith_sector_in_block{};

  sectors.back() += out_str + "\nPunteros a buckets:\n";

  for (auto b : dir.bucket_ptrs) {
    sectors.back() +=
        std::to_string(b);

    sectors.back() += '\n';

    remm_sector_bytes -= sizeof(uint32_t);

    if (remm_sector_bytes <= 0) {
      remm_sector_bytes = disk_manager->SECTOR_SIZE;
      ith_sector_in_block++;

      sectors.emplace_back();
      sectors.back() +=
          disk_manager->logic_sector_to_CHS(
              disk_manager->free_block_map.get_ith_lba(
                  curr_page_id, ith_sector_in_block)) +
          "\n" + "\nPunteros a buckets:\n";
    }
  }
  return sectors;
}

std::vector<std::string> Megatron::translate_bucket(
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
      " Size: " + std::to_string(bucket.size) + "\n" +
      "\nPrefijo: " + std::to_string((bucket.prefix));

  // Primer sector tiene la metadata de pagina
  std::vector<std::string> sectors;
  sectors.emplace_back();

  int remm_sector_bytes =
      disk_manager->SECTOR_SIZE - sizeof(Bucket::overflow_page) -
      sizeof(Bucket::local_depth) - sizeof(Bucket::size) -
      sizeof(Bucket::capacity);

  size_t ith_sector_in_block{};

  sectors.back() += out_str;

  for (size_t i{}; i < bucket.capacity; ++i) {
    if (bucket.reg_ptrs[i].page_id != disk_manager->NULL_BLOCK) {
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

      sectors.emplace_back();
      sectors.back() += disk_manager->logic_sector_to_CHS(
                            disk_manager->free_block_map.get_ith_lba(
                                curr_page_id, ith_sector_in_block)) +
                        "\n" + "\nPunteros a registros:\n";
    }
  }
  return sectors;
}
