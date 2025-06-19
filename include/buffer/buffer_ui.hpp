#ifndef BUFFER_H
#define BUFFER_H

#include "disk_manager.hpp"
#include "serial/fixed_data.hpp"
#include "serial/page_header.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <iostream>
#include <list>
#include <unordered_map>
#include <vector>

using namespace std;

struct BufferEntry {
  int frame_id;
  int page_id;
  std::vector<unsigned char> data{};
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

  DiskManager &disk_manager;
  serial::TableMetadata &table_metadata;
  // serial::PageHeader &page_header;
  // serial::FixedDataHeader &fixed_data_header;

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
        if (entry.dirty) {
          cout << "Se esta por hacer eviction a una pagina SUCIA con block_id: " << candidate << endl;
          cout << "?Deseas guardarla?(0:no, 1:si) ";

          int opcion;
          cin >> opcion;

          if (opcion == 1) {
            savePage(entry);
            cout << "Pagina SUCIA guardada." << endl;
          } else {
            cout << "Pagina SUCIA descartada." << endl;
          }
          // cout << "Escribiendo pagina " << candidate << " al disco (dirty).\n";

        } else
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
  BufferUI(int size, DiskManager &disk_manager,
           serial::TableMetadata &table_metadata)
      : max_frames(size), disk_manager(disk_manager), table_metadata(table_metadata) {
    for (int i = 0; i < size; ++i)
      frame_pool.push_back(i);
  }

  // Trabaja de la mano con carga de paginas reales
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

    // Se saca data directamente de disco
    std::vector<unsigned char> data;
    disk_manager.read_block(data, page_id);

    bool es_dirty = (operacion == 1);
    BufferEntry entry = {
        assigned_frame,
        page_id,
        std::move(data),
        operacion,
        es_dirty ? 1 : 0,
        1,
        0};

    buffer_map[page_id] = entry;
    updateLRU(page_id);
    cout << "Cargada pagina " << page_id << " en frame " << assigned_frame << ".\n";

    return true;
  }

  BufferEntry *get_Entry(int page_id) {
    if (buffer_map.count(page_id)) {
      return &buffer_map[page_id];
    }
    return nullptr;
  }

  void savePage(int page_id) {
    if (buffer_map.count(page_id)) {
      savePage(buffer_map[page_id]);
      return;
    }
    cout << "Pagina con id " << page_id << " no esta cargada en pool" << ".\n";
  }

  void savePage(BufferEntry &entry) {
    entry.dirty = 0;
    entry.operacion = 0;
    disk_manager.write_block(entry.data, entry.page_id);
  }

  void setPinFijo(int page_id, bool value) {
    if (buffer_map.count(page_id)) {
      buffer_map[page_id].pin_fijo = value;
      cout << "Pagina " << page_id << " ahora tiene pin fijo = " << (value ? "Si" : "No") << ".\n";
    }
  }

  void forceFlush() {
    for (auto it = lru_list.rbegin(); it != lru_list.rend(); ++it) {
      int candidate = *it;
      BufferEntry &entry = buffer_map[candidate];
      if (entry.dirty) {
        // cout << "Se esta por hacer eviction a una pagina SUCIA con block_id: " << candidate << endl;
        // cout << "?Deseas guardarla?(0:no, 1:si) ";
        //
        // int opcion;
        // cin >> opcion;

        // if (opcion == 1) {
        //   savePage(entry);
        cout << "Pagina SUCIA guardada." << endl;
        // } else {
        //   cout << "Pagina SUCIA descartada." << endl;
        // }
        // cout << "Escribiendo pagina " << candidate << " al disco (dirty).\n";

      } else
        cout << "Descartando pagina limpia " << candidate << ".\n";

      lru_list.erase(next(it).base());
      lru_map.erase(candidate);
      buffer_map.erase(candidate);
      return;
    }
    // cout << "No se pudo evictar ninguna pagina. Todas estan fijas.\n";
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

    cout << "Capacidad: " << lru_list.size() * disk_manager.BLOCK_SIZE << " / " << max_frames * disk_manager.BLOCK_SIZE << std::endl;
  }

  void printHitRate() const {
    cout << "\n\033[1;34m== Hit Rate ==\033[0m\n";
    cout << "Hits: " << hits << " / " << total_requests << " = "
         << (total_requests == 0 ? 0.0 : (double)hits / total_requests) << "\n";
  }

  void show_block(serial::TableMetadata &table_metadata, uint32_t block_id) {
    uint32_t curr_page_id = block_id;

    if (!buffer_map.count(curr_page_id)) {
      cout << "Pagina " << block_id << " no cargada en pool" << ".\n";
    }

    std::vector<unsigned char> &page_bytes = buffer_map.at(curr_page_id).data;
    // disk_manager.read_block(page_bytes, curr_page_id);

    auto page_bytes_it = page_bytes.begin();

    auto page_header = serial::deserialize_page_header(page_bytes_it);
    // if (page_header.n_regs == 0) // Pagina vacia
    //   continue;

    serial::FixedDataHeader fixed_data_header;
    fixed_data_header = serial::deserialize_fixed_data_header(page_bytes_it);

    show_fixed_page(table_metadata, page_header,
                    fixed_data_header, page_bytes, curr_page_id);

    // curr_page_id = page_header.next_block_id;
    // std::cout << curr_page_id << std::endl;
  }

  // Es una pagina completa, implica varios sectores, el primero es afectado por el header
  void show_fixed_page(
      serial::TableMetadata &table_metadata,
      serial::PageHeader &page_header,
      serial::FixedDataHeader &fixed_data_header,
      std::vector<unsigned char> &page_bytes, uint32_t curr_page_id) {

    int remm_sector_bytes =
        disk_manager.SECTOR_SIZE -
        serial::calculate_fixed_data_header_size(fixed_data_header); // Deberia restarse size de header
    size_t ith_sector_in_block{};

    // PageHeader
    std::string out_str{};
    out_str += "Next_page_id: " + std::to_string(page_header.next_block_id) + " ";
    out_str += "N_registers: " + std::to_string(page_header.n_regs) + " ";
    out_str += "Free_space/capacity: " + std::to_string(page_header.free_space) +
               '/' + std::to_string(disk_manager.BLOCK_SIZE - serial::calculate_fixed_data_header_size(fixed_data_header)) +
               "\n";

    // FixedDataHeader
    out_str += "Register size: " + std::to_string(fixed_data_header.reg_size) + " ";
    out_str += "Max registers: " + std::to_string(fixed_data_header.max_n_regs) + " ";
    std::string bitmap_str;
    boost::to_string(fixed_data_header.free_register_bitmap, bitmap_str);
    out_str += "Free_register_bitmap: " + bitmap_str + "\n";

    std::cout << out_str << std::endl;
    std::cout << disk_manager.logic_sector_to_CHS(
                     disk_manager.free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block))
              << std::endl;

    // disk.write_block_txt(out_str, curr_page_id);
    // disk.write_block_txt(disk.logic_sector_to_CHS(
    //                          disk.free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block)),
    //                      curr_page_id);

    for (size_t i{}; i < fixed_data_header.max_n_regs; ++i) {
      out_str = "";
      if (fixed_data_header.free_register_bitmap.at(i)) { // Registro existe
        auto register_bytes = get_ith_register_bytes(table_metadata,
                                                     page_header,
                                                     fixed_data_header, page_bytes, i);
        auto register_values = deserialize_register(table_metadata, register_bytes);

        for (auto &v : register_values)
          // disk.write
          out_str += SQL_type_to_string(v) + " ";

        // out_str+='\n';
      } else { // Registro vacio/deleted
        out_str += '\n';
      }
      remm_sector_bytes -= fixed_data_header.reg_size;
      if (remm_sector_bytes < 0) {
        remm_sector_bytes = disk_manager.SECTOR_SIZE;
        ith_sector_in_block++;
        std::cout << "\n" + disk_manager.logic_sector_to_CHS(
                                disk_manager.free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block))
                  << std::endl;
      }

      cout << out_str << std::endl;
      // disk.write_sector_txt(
      //     out_str,
      //     disk.free_block_map.get_ith_lba(curr_page_id, ith_sector_in_block));
      // disk.write_block_txt(out_str, curr_page_id);
    }
  }
  /*
   * Retorna los bytes de un registro arbitrario en una pagina fija
   */
  std::vector<unsigned char> get_ith_register_bytes(
      const serial::TableMetadata &table_metadata,
      const serial::PageHeader &page_header,
      const serial::FixedDataHeader &fixed_data_header,
      const std::vector<unsigned char> &page_bytes,
      size_t ith_reg) {

    // Calculo de byte inicial del registro
    size_t reg_offset = serial::calculate_reg_offset(fixed_data_header, ith_reg);

    // Se extraen bytes de la pagina en ese offset
    std::vector<unsigned char> register_bytes(
        page_bytes.begin() + reg_offset,
        page_bytes.begin() + reg_offset + table_metadata.max_reg_size);

    return register_bytes;
  }

  std::vector<unsigned char> serialize_register(
      const serial::TableMetadata &table_metadata,
      std::vector<std::string> &values) {
    if (values.size() != table_metadata.n_cols)
      throw std::runtime_error("Columnas a insertar diferentes a tabla");

    std::vector<unsigned char> register_bytes;
    register_bytes.reserve(table_metadata.max_reg_size);

    for (size_t col{}; col < table_metadata.n_cols; ++col) {
      auto temp_val = string_to_sql_type(values[col],
                                         table_metadata.columns[col].type,
                                         table_metadata.columns[col].max_size);

      auto sql_type_bytes = serialize_sql_type(temp_val);

      register_bytes.insert(register_bytes.end(),
                            sql_type_bytes.begin(),
                            sql_type_bytes.end());
    }

    return register_bytes;
  }

  std::vector<SQL_type> deserialize_register(
      const serial::TableMetadata &table_metadata,
      std::vector<unsigned char> &register_bytes) {

    if (register_bytes.size() > table_metadata.max_reg_size)
      throw std::runtime_error("Bytes de registro mas grande que tamanio maximo de reg");

    // std::vector<unsigned char> register_bytes;
    // register_bytes.reserve(table_metadata.max_reg_size);
    std::vector<SQL_type> register_values;

    auto register_bytes_it = register_bytes.begin();
    for (size_t col{}; col < table_metadata.n_cols; ++col) {
      auto temp_val = deserialize_sql_type(register_bytes_it,
                                           table_metadata.columns[col].type,
                                           table_metadata.columns[col].max_size);

      register_values.push_back(temp_val);
    }

    return register_values;
  }
};

#endif
