#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <list>
#include <unordered_map>
#include <vector>

using namespace std;

struct BufferEntry {
  int frame_id;
  int page_id;
  int operacion; // 0 = lectura, 1 = escritura
  int dirty;     // 0 = no, 1 = si
  int pin_count; // cantidad de pines
  bool pin_fijo; // true = no se puede evictar
};

class BufferUI {
private:
  int max_frames;
  int next_frame_id = 0;
  list<int> lru_list; // page_id del mas reciente al menos reciente
  unordered_map<int, list<int>::iterator> lru_map;
  unordered_map<int, BufferEntry> buffer_map; // page_id -> BufferEntry
  vector<int> frame_pool;
  int hits = 0;
  int total_requests = 0;

  void updateLRU(int page_id) {
    if (lru_map.count(page_id)) {
      lru_list.erase(lru_map[page_id]);
    }
    lru_list.push_front(page_id);
    lru_map[page_id] = lru_list.begin();
  }

  void evictPage() {
    for (auto it = lru_list.rbegin(); it != lru_list.rend(); ++it) {
      int candidate = *it;
      BufferEntry &entry = buffer_map[candidate];
      if (!entry.pin_fijo) {
        if (entry.dirty)
          cout << "Escribiendo pagina " << candidate << " al disco (dirty).\n";
        else
          cout << "Descartando pagina limpia " << candidate << ".\n";

        lru_list.erase(next(it).base());
        lru_map.erase(candidate);
        buffer_map.erase(candidate);
        return;
      }
    }
    cout << "No se pudo evictar ninguna pagina. Todas estan fijas.\n";
  }

public:
  BufferUI(int size) : max_frames(size) {
    for (int i = 0; i < size; ++i)
      frame_pool.push_back(i);
  }

  bool loadPage(int page_id, int operacion, bool pin_fijo = false) {
    total_requests++;
    if (buffer_map.count(page_id)) {
      hits++;
      BufferEntry &entry = buffer_map[page_id];
      entry.pin_count++;

      // Caso reads -> write, queda marcado dirty
      // Write toma prioridad sobre read
      entry.operacion = operacion;
      if (operacion) {
        entry.dirty = 1;
      }

      cout << "HIT: Pagina " << page_id << " ya esta cargada. Pin count ahora = " << entry.pin_count << "\n";
      updateLRU(page_id);
      return true;
    }

    if ((int)buffer_map.size() >= max_frames)
      evictPage();

    if ((int)buffer_map.size() >= max_frames) {
      cout << "Buffer lleno. No se pudo cargar la pagina " << page_id << ".\n";
      // total_requests--; // ?Si no se puede insertar, no agrega a total_requests?
      return false;
    }

    int assigned_frame = -1;
    for (int i = 0; i < max_frames; ++i) {
      bool ocupado = false;
      for (auto &[_, entry] : buffer_map) {
        if (entry.frame_id == i) {
          ocupado = true;
          break;
        }
      }
      if (!ocupado) {
        assigned_frame = i;
        break;
      }
    }

    bool es_dirty = (operacion == 1);
    BufferEntry entry = {
        assigned_frame,
        page_id,
        operacion,
        es_dirty ? 1 : 0,
        1,
        0};

    buffer_map[page_id] = entry;
    updateLRU(page_id);
    cout << "Cargada pagina " << page_id << " en frame " << assigned_frame << ".\n";

    return true;
  }

  void setPinFijo(int page_id, bool value) {
    if (buffer_map.count(page_id)) {
      buffer_map[page_id].pin_fijo = value;
      cout << "Pagina " << page_id << " ahora tiene pin fijo = " << (value ? "Si" : "No") << ".\n";
    }
  }

  void printBuffer() const {
    cout << "\n\033[1;34m== Estado del Buffer ==\033[0m\n";
    cout << "\033[4mFRAME\tPAGE\tOP\tDIRTY\tPIN_CNT\tPIN_FIJO\033[0m\n";
    vector<const BufferEntry *> ordenado(max_frames, nullptr);
    for (const auto &[_, entry] : buffer_map) {
      ordenado[entry.frame_id] = &entry;
    }
    for (int i = 0; i < max_frames; ++i) {
      if (ordenado[i]) {
        const BufferEntry *e = ordenado[i];
        cout << e->frame_id << "\t" << e->page_id << "\t"
             << ((e->operacion) ? "E" : "L") << "\t" << e->dirty << "\t"
             << e->pin_count << "\t";

        if (e->pin_fijo) {
          cout << "\033[31mSi\033[0m"; // Rojo
        } else {
          cout << "No";
        }
        cout << "\n";
      } else {
        cout << i << "\t<vacio>\n";
      }
    }
  }

  void printLRU() const {
    cout << "\n\033[1;34m== Lista LRU (mas reciente -> menos reciente) ==\033[0m\n";
    for (int page_id : lru_list) {
      bool is_fixed = buffer_map.at(page_id).pin_fijo;

      if (is_fixed) {
        cout << "-> \033[31m" << page_id << "\033[0m "; // Rojo
      } else {
        cout << "-> " << page_id << " ";
      }
    }
    cout << "\n";
  }

  void printHitRate() const {
    cout << "\n\033[1;34m== Hit Rate ==\033[0m\n";
    cout << "Hits: " << hits << " / " << total_requests << " = "
         << (total_requests == 0 ? 0.0 : (double)hits / total_requests) << "\n";
  }
};

#endif
