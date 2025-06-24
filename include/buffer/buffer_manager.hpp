#pragma once

#include "disk_manager.hpp"
#include "frame.hpp"
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

class BufferManager {
public:
  BufferManager(size_t capacity, bool is_clock, std::unique_ptr<DiskManager> &disk_manager)
      : capacity(capacity), disk_manager(disk_manager.get()), is_clock(is_clock) {
    if (is_clock)
      clock.resize(capacity, disk_manager->NULL_BLOCK);
  }

  // Carga de pagina, necesariamente incrementa pin_count
  Frame &load_pin_page(size_t page_id) {
    auto it = frame_map.find(page_id);

    // Si la página no está en buffer, cargarla
    if (it == frame_map.end()) {
      if (frame_map.size() >= capacity) {
        evict_page();
      }
      load_page(page_id);
      it = frame_map.find(page_id);
    }

    // Mueve frame al frente
    if (!is_clock)
      lru_list.splice(lru_list.begin(), lru_list, it->second.lru_it);
    else {
      it->second.reference_bit = 1;
    }
    it->second.pin_count++;

    return *(it->second.frame);
  }

  // Libera frame
  void free_unpin_page(size_t page_id, bool is_dirty = false) {
    auto it = frame_map.find(page_id);
    if (it == frame_map.end()) {
      throw std::runtime_error("Free de pagina no en buffer");
    }

    it->second.pin_count--;
    if (is_dirty) {
      it->second.frame->dirty = true;
    }
  }

  // Escribe todas las páginas sucias a disco, no limpia buffer
  void flush_all() {
    for (auto &[page_id, entry] : frame_map) {
      if (entry.frame->dirty) {
        disk_manager->write_block(entry.frame->page_bytes, page_id);
        entry.frame->dirty = false;
      }
    }
  }

  size_t get_hits() { return hits; }

  size_t get_total_accesses() { return total; }

private:
  void load_page(size_t page_id) {
    std::vector<unsigned char> data;

    // TODO: algun check de bloque_id valido?
    disk_manager->read_block(data, page_id);

    auto frame = std::make_unique<Frame>(page_id, std::move(data));

    if (!is_clock) {
      lru_list.push_front(page_id);

      frame_map[page_id] = {
          std::move(frame), lru_list.begin(),
          0 // pin_count en 0, se incrementa al hacer pin
      };
    } else {
      frame_map[page_id] = {
          std::move(frame), lru_list.end(),
          0 // pin_count en 0, se incrementa al hacer pin
      };
    }
  }

  void evict_page() {
    if (is_clock) {

      for (auto &e : frame_map) {
      }

      hand += 1;

    } else {
      // Busqueda de pagina menos usada que no este pinned
      for (auto it = lru_list.rbegin(); it != lru_list.rend(); ++it) {
        auto &entry = frame_map[*it];

        if (entry.pin_count == 0) {
          if (entry.frame->dirty) {
            disk_manager->write_block(entry.frame->page_bytes, *it);
          }

          frame_map.erase(*it);
          lru_list.erase(std::next(it).base()); // Rev_it a forward
          return;
        }
      }
    }

    throw std::runtime_error("Todas las paginas estan pineadas, imposible insertar");
  }

  DiskManager *disk_manager{};
  size_t capacity;
  bool is_clock{};

  int hits, total;
  size_t hand{};

  std::list<size_t> lru_list;
  std::vector<size_t> clock;
  std::unordered_map<size_t, BufferFrame> frame_map;
};
