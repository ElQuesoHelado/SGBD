#include "buffer/buffer_manager.hpp"

void BufferManager::evict_page() {
  int success{-1};
  if (is_clock) {
    if (verbose)
      success = evict_page_Clock_verbose();
    else
      success = evict_page_Clock();
  } else {
    if (verbose)
      success = evict_page_LRU_verbose();
    else
      success = evict_page_LRU();
  }

  if (success == -1)
    throw std::runtime_error("Todas las paginas estan pineadas, imposible insertar");
}

int BufferManager::evict_page_LRU() {
  // Pagina menos usada que no este pinned
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

      if (buffer_frame.frame->dirty)
        disk_manager->write_block(buffer_frame.frame->page_bytes, *it);

      frame_map.erase(*it);
      lru_list.erase(std::next(it).base()); // Rev_it a forward
      return 0;
    }
  }
  return -1;
}

int BufferManager::evict_page_LRU_verbose() {
  // Pagina menos usada que no este pinned
  while (true) {
    for (auto it = lru_list.rbegin(); it != lru_list.rend(); ++it) {
      auto &buffer_frame = frame_map[*it];

      // Stack de frame vacio, no hay reads ni writes pendientes
      if (buffer_frame.pin_count == 0 && buffer_frame.fixed_pin == 0) {
        // Marca frame_slot como libre
        for (size_t i = 0; i < frame_slots.size(); ++i) {
          if (frame_slots[i] == *it) {
            frame_slots[i] = disk_manager->NULL_BLOCK;
            break;
          }
        }

        // Se hace espacio en lista/map
        frame_map.erase(*it);
        lru_list.erase(std::next(it).base()); // Rev_it a forward
        return 0;

      } else if (buffer_frame.fixed_pin == 0 && buffer_frame.pin_count > 0) { // Caso fixed pin, se deja intacto
        int opcion;

        std::cout << "Pagina con page_id: " << *it << " tiene " << buffer_frame.ops_stack.size() << " procesos activos" << std::endl;
        std::cout << "?Deseas terminar el proceso mas reciente?(0:no, 1:si) ";
        std::cin >> opcion;

        if (opcion != 1)
          continue;

        auto curr_op = buffer_frame.ops_stack.back();

        if (curr_op == 'W') {
          std::cout << "Dicho proceso es de escritura" << std::endl;
          std::cout << "?Deseas guardar la pagina?(0:no, 1:si) ";

          int opcion;
          std::cin >> opcion;

          if (opcion == 1) {
            disk_manager->write_block(buffer_frame.frame->page_bytes, *it);
            buffer_frame.frame->dirty = false;
            std::cout << "Pagina SUCIA guardada." << std::endl;
          } else {
            std::cout << "Pagina SUCIA descartada." << std::endl;
          }

        } else {
          std::cout << "Proceso de lectura terminado en " << *it << ".\n";
        }

        buffer_frame.ops_stack.pop_back();
        if (!buffer_frame.ops_stack.empty())
          buffer_frame.frame->dirty = buffer_frame.ops_stack.back() == 'W';
        buffer_frame.pin_count--;

      } else {
        std::cout << "Pagina " << *it << " pineada" << ".\n";
      }
    }
  }

  return -1;
}

int BufferManager::evict_page_Clock() {
  size_t attempts{};
  // TODO: Ver comportamiento
  while (attempts < capacity * 10) { // max vueltas, caso pincounts altos, deberia de fallar
    size_t current_page = frame_slots[clock_hand];

    if (current_page == disk_manager->NULL_BLOCK) {
      clock_hand = (clock_hand + 1) % capacity;
      continue;
    }

    auto it = frame_map.find(current_page);
    auto &entry = it->second;
    if (entry.pin_count > 0 || entry.fixed_pin > 0) {
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

    if (entry.frame->dirty)
      disk_manager->write_block(entry.frame->page_bytes, current_page);

    frame_map.erase(current_page);
    frame_slots[clock_hand] = disk_manager->NULL_BLOCK;
    clock_hand = (clock_hand + 1) % capacity;
    return 0;
  }

  return -1;
}

// Realiza EVICTION, siempre mata a alguien(caso haya espacio disponible igual se dispara)
int BufferManager::evict_page_Clock_verbose() {
  size_t attempts{};
  while (true) { // Saboteamos loop de clock para reducir pincounts a la par

    size_t current_page = frame_slots[clock_hand];
    std::cout << "Mano en frame 👉🏾:  " << clock_hand << " con id: " << current_page << "\n";

    if (current_page == disk_manager->NULL_BLOCK) {
      std::cout << "\tFrame vacio, se ignora" << std::endl;
      clock_hand = (clock_hand + 1) % capacity;
      continue;
    }

    auto it = frame_map.find(current_page);
    auto &entry = it->second;

    if (entry.fixed_pin != 0) {
      std::cout << "\tFrame fijado, se ignora" << std::endl;
      clock_hand = (clock_hand + 1) % capacity;
      continue;
    }

    if (entry.pin_count > 0) { // Se puede reducir pincount
      std::cout << "\tPincount: " << entry.pin_count << ".\n";

      int opcion;
      std::cout << "\tTiene " << entry.ops_stack.size() << " procesos activos" << std::endl;
      std::cout << "\t?Deseas terminar el proceso mas reciente?(0:no, 1:si) ";
      std::cin >> opcion;

      if (opcion == 1) {
        auto curr_op = entry.ops_stack.back();
        // std::cout << "Pincount reducido en 1\n";
        if (curr_op == 'W') {
          std::cout << "\t\tDicho proceso es de escritura" << std::endl;
          std::cout << "\t\t?Deseas guardar la pagina?(0:no, 1:si) ";

          int opcion;
          std::cin >> opcion;

          if (opcion == 1) {
            disk_manager->write_block(entry.frame->page_bytes, current_page);
            entry.frame->dirty = false;
            std::cout << "\t\tPagina SUCIA guardada." << std::endl;
          } else {
            std::cout << "\t\tPagina SUCIA descartada." << std::endl;
          }

        } else {
          std::cout << "\t\tProceso de lectura terminado en " << current_page << ".\n";
        }

        entry.ops_stack.pop_back();
        if (!entry.ops_stack.empty())
          entry.frame->dirty = entry.ops_stack.back() == 'W';
        entry.pin_count--;
      }

      attempts++;
      clock_hand = (clock_hand + 1) % capacity;
      continue;
    } else if (entry.reference_bit) {
      std::cout << "\tPagina " << current_page << " con reference_bit 1, cambia a 0" << ".\n";
      entry.reference_bit = false;
      attempts++;
      clock_hand = (clock_hand + 1) % capacity;
      continue;
    }

    std::cout << "Pagina " << current_page << " se va a eliminar" << ".\n";

    if (entry.frame->dirty) {
      std::cout << "\tSe esta por hacer eviction a una pagina SUCIA con page_id: " << current_page << std::endl;
      std::cout << "\t?Deseas guardarla?(0:no, 1:si) ";

      int opcion;
      std::cin >> opcion;

      if (opcion == 1) {
        disk_manager->write_block(entry.frame->page_bytes, current_page);
        std::cout << "\t\tPagina SUCIA guardada." << std::endl;
      } else {
        std::cout << "\t\tPagina SUCIA descartada." << std::endl;
      }

    } else {
      std::cout << "\tDescartando pagina limpia " << current_page << ".\n";
    }

    frame_map.erase(current_page);
    frame_slots[clock_hand] = disk_manager->NULL_BLOCK;
    clock_hand = (clock_hand + 1) % capacity;
    return 0;
  }

  return -1;
}
