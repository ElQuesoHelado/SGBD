#include "buffer/buffer_manager.hpp"

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

// Para verbose/ui
Frame &BufferManager::load_pin_page_push_op(size_t page_id, char op) {
  auto &frame = load_pin_page(page_id);
  auto &buffer_frame = frame_map.find(page_id)->second;

  // Caso se tenga un write en top stack, se tiene que escribir/descartar
  if (!buffer_frame.ops_stack.empty() && buffer_frame.ops_stack.back() == 'W') {
    int opcion;
    std::cout << "㊗️‼️ Pagina con page_id: " << page_id << " tiene como ultima operacion un write ‼️㊗️" << std::endl;
    std::cout << "?Deseas escribir dicho proceso?(0:no, 1:si) ";
    std::cin >> opcion;

    if (opcion == 1) {
      disk_manager->write_block(frame.page_bytes, page_id);
      std::cout << "Proceso guardado." << std::endl;
    } else {
      std::cout << "Proceso descartado." << std::endl;
    }

    buffer_frame.ops_stack.pop_back();
    if (!buffer_frame.ops_stack.empty())
      buffer_frame.frame->dirty = buffer_frame.ops_stack.back() == 'W';
    buffer_frame.pin_count--;
  }

  buffer_frame.ops_stack.push_back(op);

  frame.dirty = (op == 'W');

  return frame;
}

void BufferManager::load_page(size_t page_id, bool fixed_pin) {
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
    // clock_hand = (free_slot + 1) % capacity;
  }
  if (verbose)
    std::cout << "Pagina cargada en frame " << free_slot << ".\n";

  if (fixed_pin)
    set_fixed_pin(page_id, true);
}
