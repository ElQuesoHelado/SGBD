#include "buffer/buffer_manager.hpp"

BufferManager::BufferManager(size_t capacity, bool is_clock, std::unique_ptr<DiskManager> &disk_manager)
    : capacity(capacity), disk_manager(disk_manager.get()), is_clock(is_clock), frame_slots(capacity, disk_manager->NULL_BLOCK) {
}

// Carga de pagina, necesariamente incrementa pin_count
Frame &BufferManager::load_pin_page(size_t page_id) {
  total++;
  auto it = frame_map.find(page_id);

  // Si la página no está en buffer, cargarla
  if (it == frame_map.end()) {
    if (frame_map.size() >= capacity) {
      evict_page();
    }
    load_page(page_id);
    it = frame_map.find(page_id);
  } else {
    hits++;
    if (verbose)
      std::cout << "HIT: Pagina " << page_id << "\n";
  }

  // Mueve frame al frente
  if (!is_clock)
    lru_list.splice(lru_list.begin(), lru_list, it->second.lru_it);
  else {
    it->second.reference_bit = 1;
  }
  it->second.pin_count++;

  if (verbose)
    std::cout << "Pagina " << page_id
              << " ahora con pin count = " << it->second.pin_count << "\n";

  return *(it->second.frame);
}

// Libera frame
void BufferManager::free_unpin_page(size_t page_id, bool is_dirty = false) {
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
void BufferManager::flush_all() {
  for (auto &[page_id, entry] : frame_map) {
    if (entry.frame->dirty) {
      disk_manager->write_block(entry.frame->page_bytes, page_id);
      entry.frame->dirty = false;
    }
  }
}

void BufferManager::set_fixed_pin(int page_id, bool value) {
  if (frame_map.count(page_id)) {
    frame_map[page_id].fixed_pin = value;
    if (verbose)
      std::cout << "Pagina " << page_id << " con valor de pin fijo = " << (value ? "Si" : "No") << ".\n";
  }
}

size_t BufferManager::get_hits() { return hits; }

size_t BufferManager::get_total_accesses() { return total; }

size_t BufferManager::find_free_slot() {
  // Para LRU o Clock: encontrar primer NULL_BLOCK
  for (size_t i = 0; i < frame_slots.size(); ++i) {
    if (frame_slots[i] == disk_manager->NULL_BLOCK) {
      return i;
    }
  }
  return disk_manager->NULL_BLOCK;
}

void BufferManager::load_page(size_t page_id) {
  std::vector<unsigned char> data;

  // TODO: algun check de bloque_id valido?
  disk_manager->read_block(data, page_id);

  auto frame = std::make_unique<Frame>(page_id, std::move(data));

  size_t free_slot = find_free_slot();
  if (free_slot == disk_manager->NULL_BLOCK) {
    throw std::runtime_error("Error en eviction");
  }

  frame_slots[free_slot] = page_id;

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
    clock_hand = (free_slot + 1) % capacity;
  }
}

void BufferManager::evict_page() {
  if (is_clock) {
    size_t attempts{};
    // TODO: Ver comportamiento
    while (attempts < capacity * 10) { // max vueltas, caso pincounts altos
      size_t current_page = frame_slots[clock_hand];

      if (current_page == disk_manager->NULL_BLOCK) {
        clock_hand = (clock_hand + 1) % capacity;
        continue;
      }

      // Caso slots desincronizado con frame_map
      auto it = frame_map.find(current_page);
      if (it == frame_map.end()) {
        frame_slots[clock_hand] = disk_manager->NULL_BLOCK;
        clock_hand = (clock_hand + 1) % capacity;
        continue;
      }

      auto &entry = it->second;
      if (entry.pin_count > 0) {
        attempts++;
        clock_hand = (clock_hand + 1) % capacity;
        continue;
      }
      if (entry.reference_bit) {
        entry.reference_bit = false;
        attempts++;
        clock_hand = (clock_hand + 1) % capacity;
        continue;
      }

      // Evicción (igual que antes)
      if (entry.frame->dirty) {
        disk_manager->write_block(entry.frame->page_bytes, current_page);
      }
      frame_map.erase(current_page);
      frame_slots[clock_hand] = disk_manager->NULL_BLOCK; // Cambio 4: Usa frame_slots
      clock_hand = (clock_hand + 1) % capacity;           // Cambio 5: Avanza hand después de evicción
      return;
    }
  } else {
    // Busqueda de pagina menos usada que no este pinned
    for (auto it = lru_list.rbegin(); it != lru_list.rend(); ++it) {
      auto &buffer_frame = frame_map[*it];

      if (buffer_frame.pin_count == 0 && buffer_frame.fixed_pin == 0) {
        // Marca frame_slot como libre
        for (size_t i = 0; i < frame_slots.size(); ++i) {
          if (frame_slots[i] == *it) {
            frame_slots[i] = disk_manager->NULL_BLOCK;
            break;
          }
        }

        if (buffer_frame.frame->dirty) {
          disk_manager->write_block(buffer_frame.frame->page_bytes, *it);
        }

        frame_map.erase(*it);
        lru_list.erase(std::next(it).base()); // Rev_it a forward
        return;
      }
    }
  }

  throw std::runtime_error("Todas las paginas estan pineadas, imposible insertar");
}
