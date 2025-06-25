#pragma once

#include "disk_manager.hpp"
#include "frame.hpp"
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

class BufferManager {
public:
  BufferManager(size_t capacity, bool is_clock, std::unique_ptr<DiskManager> &disk_manager);

  // Carga de pagina, necesariamente incrementa pin_count
  Frame &load_pin_page(size_t page_id);

  // Libera frame
  void free_unpin_page(size_t page_id, bool is_dirty);

  // Escribe todas las páginas sucias a disco, no limpia buffer
  void flush_all();

  // Flush + reset de tanto lru_list y clock vector/hand
  // TODO: Implementar, li
  void flush_all_clean();

  void set_fixed_pin(int page_id, bool value);

  size_t get_hits();
  size_t get_total_accesses();
  bool is_buffer_clock();

  // UI
  void print_buffer_LRU() const;
  void print_buffer_clock() const;
  void print_LRU_list() const;
  void print_hit_rate() const;

private:
  size_t find_free_slot();

  // Operaciones directas en disco
  void load_page(size_t page_id, bool fixed_pin = false);
  void evict_page();

  DiskManager *disk_manager{};
  size_t capacity{};
  bool is_clock{};
  bool verbose{};

  int hits{}, total{};
  size_t clock_hand{};

  std::list<size_t> lru_list;

  // Estructura usada para mantener un orden de frames/ operaciones en clock
  std::vector<size_t> frame_slots;

  std::unordered_map<size_t, BufferFrame> frame_map;
};
