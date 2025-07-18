#pragma once

#include "disk_manager.hpp"
#include "frame.hpp"
#include <cstddef>
#include <memory>
#include <vector>

class BufferManager {
public:
  BufferManager(size_t capacity, bool is_clock, std::unique_ptr<DiskManager> &disk_manager);

  Frame &get_load_free_frame();

  // Carga de pagina, necesariamente incrementa pin_count
  Frame &load_pin_page(size_t page_id);

  Frame &load_pin_page_push_op(size_t page_id, char op);

  // Libera frame
  void free_unpin_page(size_t page_id, bool is_dirty);

  // Escribe todas las páginas sucias a disco, no limpia buffer
  void flush_all();

  // Limpia paginas internas, no parametros
  void clear();

  void set_fixed_pin(int page_id, bool value);

  size_t get_hits();
  size_t get_total_accesses();
  bool is_buffer_clock();

  // UI
  void print_buffer_LRU() const;
  void print_buffer_clock() const;
  void print_LRU_list() const;
  void print_hit_rate() const;

  void print_page(size_t page_id);

  void set_verbose(bool is_verbose) { verbose = is_verbose; };

  std::vector<unsigned char> get_page_bytes(size_t page_id) {
    std::vector<unsigned char> res{};
    auto it = frame_map.find(page_id);

    if (it != frame_map.end()) {
      res = it->second.frame->page_bytes;
    }

    return res;
  }

private:
  size_t find_free_slot();

  // Operaciones directas en disco
  void load_page(size_t page_id, bool fixed_pin = false);
  void evict_page();
  int evict_page_LRU();
  int evict_page_LRU_verbose();
  int evict_page_Clock();
  int evict_page_Clock_verbose();

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
