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

std::vector<uint32_t> Megatron::translate_bptree_node_page(
    serial::TableMetadata &table_metadata, BPTree &tree,
    uint32_t node_id) {
  std::vector<uint32_t> pages{};

  std::vector<unsigned char> page_bytes;
  disk_manager->read_block(page_bytes, node_id);

  BPNode node(page_bytes, tree.key_type,
              tree.key_size, node_id,
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

  sectors.back() += out_str +
                    disk_manager->logic_sector_to_CHS(
                        disk_manager->free_block_map.get_ith_lba(
                            node.node_id, ith_sector_in_block)) +
                    "\n";

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
                                node_id, ith_sector_in_block)) +
                        "\n";
    }
  }

  sectors.back() += std::to_string(node.ptrs[node.n_keys]) + "\n";

  size_t i{};
  std::string page_str{};
  for (auto &e : sectors) {
    page_str += e;
    disk_manager->write_sector_txt(
        e,
        disk_manager->free_block_map.get_ith_lba(node_id, i));
    // disk_manager->write_block_txt(e, page_id);
    i++;
  }

  disk_manager->write_block_txt(page_str, node_id);

  pages.push_back(node.node_id);
  if (!node.is_leaf) {
    for (size_t i{}; i <= node.n_keys; ++i) {
      auto res = translate_bptree_node_page(table_metadata, tree, node.ptrs[i]);
      pages.insert(pages.end(), res.begin(), res.end());
    }
  }

  return pages;
}
