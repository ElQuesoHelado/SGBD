#pragma once

#include "disk_manager.hpp"
#include "frame.h"
#include <cstddef>
#include <list>
class BufferManager {

  DiskManager &disk_manager;
  size_t capacity;

  std::list<size_t> lru_list;

  // Mapeamos directamente page_id a su posicion en lista y datos en heap
  std::unordered_map<
      size_t,
      std::pair<typename std::list<size_t>::iterator, std::unique_ptr<Frame>>>
      frame_map;

public:
  // Flush total, solo llamado en destruccion
  // TODO: ?Llamado al interactuar con UI?, esto para tener un buffer pool limpio?
  // TODO: ?"limpiar buffer"/Vaciar?
  void flush_all_frames() {
    for (auto &item : frame_map) {
      if (item.second.second->dirty) {
        disk_manager.write_block(item.second.second->page_bytes, item.first);
        item.second.second->dirty = false;
      }
    }
  }

  Frame &get_frame(size_t block_id) {
    auto it = frame_map.find(block_id);

    if (it != frame_map.end()) {
      // Mover al frente
      lru_list.splice(lru_list.begin(), lru_list, it->second.first);
      return *(it->second.second);
    }

    // return load_page(block_id);
  }

  BufferManager(size_t capacity, DiskManager &disk_manager)
      : capacity(capacity), disk_manager(disk_manager) {
  }

  ~BufferManager() {
    // fush_all_frames();
  }
};
