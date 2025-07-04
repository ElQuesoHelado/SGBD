#include "buffer/buffer_manager.hpp"
#include <print>

BufferManager::BufferManager(size_t capacity, bool is_clock, std::unique_ptr<DiskManager> &disk_manager)
    : capacity(capacity), disk_manager(disk_manager.get()), is_clock(is_clock), frame_slots(capacity, disk_manager->NULL_BLOCK) {
}

bool BufferManager::is_buffer_clock() { return is_clock; }

// Libera frame, reduce pin_count
void BufferManager::free_unpin_page(size_t page_id, bool is_dirty = false) {
  auto it = frame_map.find(page_id);
  if (it == frame_map.end()) {
    throw std::runtime_error("Free de pagina no en buffer");
  }

  if (it->second.pin_count > 0)
    it->second.pin_count--;

  if (is_dirty) {
    it->second.frame->dirty = true;
  }
}

// Escribe todas las páginas sucias a disco, no limpia buffer
void BufferManager::flush_all() {
  for (auto &[page_id, entry] : frame_map) {
    if (entry.frame->dirty) {
      disk_manager->write_block(entry.frame->page_bytes, page_id);
      entry.frame->dirty = false;
    }
  }
}

void BufferManager::clear() {
  for (auto &e : frame_slots)
    e = disk_manager->NULL_BLOCK;

  frame_map.clear();
  lru_list.clear();
  clock_hand = 0;
}

void BufferManager::set_fixed_pin(int page_id, bool value) {
  if (frame_map.count(page_id)) {
    frame_map[page_id].fixed_pin = value;
    if (verbose)
      std::println("Pagina {} con valor de pin fijo = {}", page_id, (value ? "Si" : "No"));
  }
}

size_t BufferManager::get_hits() { return hits; }

size_t BufferManager::get_total_accesses() { return total; }

size_t BufferManager::find_free_slot() {
  // Para LRU o Clock encontrar slot nulo
  for (size_t i = 0; i < frame_slots.size(); ++i) {
    if (frame_slots[i] == disk_manager->NULL_BLOCK) {
      return i;
    }
  }
  return disk_manager->NULL_BLOCK;
}
