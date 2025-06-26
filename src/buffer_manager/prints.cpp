#include "buffer/buffer_manager.hpp"
#include "buffer/frame.hpp"

void BufferManager::print_buffer_LRU() const {
  std::cout << "\n\033[1;34m== Estado del Buffer ==\033[0m\n";
  std::cout << "\033[4mFRAME\tPAGE\tDIRTY\tPIN_CNT\tPIN_FIJO\tOP\033[0m\n";

  for (int i = 0; i < capacity; ++i) {
    if (frame_slots[i] != disk_manager->NULL_BLOCK) {
      auto it = frame_map.find(frame_slots[i]); // FIXME: .end()?
      const BufferFrame &e = it->second;
      std::cout << i << "\t" << e.frame->page_id << "\t"
                << e.frame->dirty << "\t"
                << e.pin_count << "\t";

      if (e.fixed_pin) {
        std::cout << "\033[31mSi\033[0m\t"; // Rojo
      } else {
        std::cout << "No\t";
      }

      std::cout << "\t";

      for (auto op : it->second.ops_stack) {
        std::cout << op;
      }

      std::cout << "\n";
    } else {
      std::cout << i << "\t<vacio>\n";
    }
  }
}

void BufferManager::print_buffer_clock() const {
  std::cout << "\n\033[1;34m== Estado del Buffer ==\033[0m\n";
  std::cout << "\033[4mHAND\tFRAME\tPAGE\tDIRTY\tREF_BIT\tPIN_CNT\tPIN_FIJO\tOP\033[0m\n";

  for (int i = 0; i < capacity; ++i) {
    if (frame_slots[i] != disk_manager->NULL_BLOCK) {
      auto it = frame_map.find(frame_slots[i]); // FIXME: .end()?
      const BufferFrame &e = it->second;
      std::cout << ((clock_hand == i) ? "👉🏽\t" : "\t") << i << "\t" << e.frame->page_id << "\t"
                << e.frame->dirty << "\t" << e.reference_bit << "\t" << e.pin_count << "\t";

      if (e.fixed_pin) {
        std::cout << "\033[31mSi\033[0m\t"; // Rojo
      } else {
        std::cout << "No\t";
      }

      std::cout << "\t";
      for (auto op : it->second.ops_stack) {
        std::cout << op;
      }

      std::cout << "\n";
    } else {
      std::cout << ((clock_hand == i) ? "👉🏽\t" : "\t") << i << "\t<vacio>\n";
    }
  }
}

void BufferManager::print_LRU_list() const {
  std::cout << "\n\033[1;34m== Lista LRU (mas reciente -> menos reciente) ==\033[0m\n";
  for (int page_id : lru_list) {
    bool is_fixed = frame_map.at(page_id).fixed_pin;

    if (is_fixed) {
      std::cout << "-> \033[31m" << page_id << "\033[0m "; // Rojo
    } else {
      std::cout << "-> " << page_id << " ";
    }
  }
  std::cout << "\n";

  std::cout << "Capacidad: " << lru_list.size() * disk_manager->BLOCK_SIZE
            << " / " << capacity * disk_manager->BLOCK_SIZE << std::endl;
}

void BufferManager::print_hit_rate() const {
  std::cout << "\n\033[1;34m== Hit Rate ==\033[0m\n";
  std::cout << "Hits: " << hits << " / " << total << " = "
            << (total == 0 ? 0.0 : (double)hits / total) << "\n";
}

void BufferManager::print_page(size_t page_id) {
}
