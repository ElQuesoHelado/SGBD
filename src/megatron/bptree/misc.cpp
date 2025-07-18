#include "bptree/bpnode.hpp"
#include "bptree/bptree.hpp"
#include "megatron.hpp"
#include <cstddef>
#include <cstdint>

size_t Megatron::calculate_btree_order(size_t key_size) {
  size_t meta_size = sizeof(BPNode::is_leaf) + sizeof(BPNode::n_keys);
  size_t numerator = disk_manager->BLOCK_SIZE - meta_size;

  size_t entry_size = key_size + sizeof(uint32_t) + sizeof(uint16_t);
  size_t denominator = 2 * entry_size;

  return numerator / denominator;
}
