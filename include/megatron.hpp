#pragma once

#include "buffer/buffer_manager.hpp"
#include "buffer/buffer_ui.hpp"
#include "disk_manager.hpp"
#include "serial/fixed_data.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include "types.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

class Megatron {
  std::unique_ptr<DiskManager> disk_manager{};
  std::unique_ptr<BufferManager> buffer_manager{};
  std::unique_ptr<BufferUI> buffer_ui{};

  static inline std::atomic<bool> global_shutdown{false};

  size_t n_sectors_in_block;

  //====
  // Funciones para llenado de todo campo de headers/metadata,
  // internamente se realizan conversiones y algunos calculos
  //====

  /*
   * Se procesan datos para tener una tabla vacia
   * @note Los ptrs iniciales a paginas son vacios
   */
  void init_table_metadata(serial::TableMetadata &table_metadata,
                           std::string name, uint32_t block_id,
                           std::vector<std::pair<std::string, std::string>> &columns);

  void init_page_header(serial::PageHeader &page_header, uint32_t initial_free_space);

  void init_fixed_data_header(serial::TableMetadata &table_metadata, serial::FixedDataHeader &fixed_data_header);
  void init_slotted_data_header(serial::TableMetadata &table_metadata, serial::SlottedDataHeader &slotted_data_header);

  // Guarda toda pagina sucia y free_space_bitmap, setea managers a null
  void clean_managers();

public:
  // ====
  // Operaciones SQL genericas
  // ====

  /*
   * @brief Crea tabla en sector 1
   * @param std::string nombre_tabla
   * @param cols Vector de pares de strings que describen columnas
   * @return si creacion fue posible(caso nombre ya existente)
   * Una table nueva esto implica su escritura de metadata en particion de DBMS,
   * la creacion de 1 file inicial en particion de datos
   * TODO: Cambio dinamico de size de file y paginas
   * ??Cambio a pair de std::array??
   * Dos tipos de files(fixed y variable)
   */
  bool create_table(std::string name, std::vector<std::pair<std::string, std::string>> &col_name_type);
  // Copia una tabla
  bool create_table(std::string name, serial::TableMetadata &copied_table_metadata);

  // void select(Relation &relation, std::vector<std::string> &fields, std::vector<std::string> &conditions);
  void select(std::string &table_name, std::string &col_name, std::string &condition);
  void select_fixed(serial::TableMetadata &table_metadata, size_t col_index, SQL_type &cond_val);
  void select_slotted(serial::TableMetadata &table_metadata, size_t col_index, SQL_type &cond_val);
  // void update(std::string cond_col_name, std::string condition, std::string col_name, std::string new_value);

  void delete_reg(std::string &table_name, std::string &col_name, std::string &condition);
  void delete_fixed(serial::TableMetadata &table_metadata, size_t col_index, SQL_type &cond_val);
  void delete_slotted(serial::TableMetadata &table_metadata, size_t col_index, SQL_type &cond_val);

  void delete_nth_reg(std::string &table_name, size_t nth);
  void delete_nth_fixed(std::vector<unsigned char> &page_bytes, size_t nth);
  void delete_nth_slotted(std::vector<unsigned char> &page_bytes, size_t nth);

  void insert(std::string table_name, std::vector<std::string> &values);
  void insert_fixed(serial::TableMetadata &table_metadata, std::vector<std::string> &values);
  void insert_slotted(serial::TableMetadata &table_metadata, std::vector<std::string> &values);

  void select_save(std::string table_name, std::string col_name, std::string condition, std::string new_table_name);

  void find_nth_reg(std::string &table_name, size_t nth);

  /*
   * @brief Creacion y escritura de nueva tabla descrita en csv
   * @note CSV con formato:
   * NombreRelacion,,,...,
   * camp1#tipo1,camp2#tipo2,...
   * dato1,dato2,...
   */
  void load_CSV(std::string csv_path, std::string table_name, size_t n_regs = 0);
  float table_size(std::string name);

  // Helprs
  // void print_relation(Relation &relation);
  size_t char_size(std::string type);

  /*
   * Une buffers de sectores individuales, produce un bloque continuo
   */
  std::vector<unsigned char> merge_sectors_to_block(std::vector<std::vector<unsigned char>> &sectors_bytes);
  std::vector<std::vector<unsigned char>> split_block_to_sectors(std::vector<unsigned char> &&block_bytes);

  // =============================
  // Operaciones directas en disco
  // =============================

  // Determina si un bloque tiene capacidad de insertar al menos un registro
  // el tamanio del registro esta dado internamente en el header de cada sector
  // @return -1 si ningun sector es disponible, sino de [0, n_sectors_in_block)
  // int is_fixed_block_insertable(const serial::Block &block);
  // int is_slotted_block_insertable(const serial::Block &block, size_t reg_size);

  /*
   * Se busca tabla en sector 1 del disco
   */
  bool search_table(std::string name, serial::TableMetadata &table_metadata);

  size_t write_table_metadata(serial::TableMetadata &metadata);

  /*
   * Agrega header vacio a sector, este depende de la tabla asignada
   *
   * TODO: headers solo en primer sector de bloque
   */
  // FIXME: cambios a escribir tambien pageheader
  void write_empty_fixed_data_header(uint32_t block_id, serial::FixedDataHeader &fixed_data_header);
  void write_empty_slotted_data_header(uint32_t block_id, serial::SlottedDataHeader &slotted_data_header);

  // =====================
  // Utils
  // =====================

  // ===
  // Funciones de interfaz
  // ===
  void ui_load_disk();
  void ui_new_disk();
  void ui_new_table();
  void ui_select_table();
  void ui_select_table_condition();
  void ui_insert_data();
  void ui_update_reg();
  void ui_delete_data();
  void ui_delete_nth();
  void ui_load_csv();
  void ui_load_n_regs_csv();
  void ui_find_reg();
  void ui_show_table_metadata();
  void ui_interact_buffer_manager();

  void new_disk(std::string disk_name, size_t surfaces, size_t tracks, size_t sectors,
                size_t bytes, size_t sectors_block, size_t n_frames, bool is_clock);
  void load_disk(std::string disk_name, size_t n_frames, bool is_clock);

  void set_buffer_manager_frames();

  // =====
  // Operaciones de buscar un bloque
  // =====

  // En base a un header se avanza al siguiente bloque
  std::vector<unsigned char> advance_fixed(serial::FixedDataHeader &header);
  std::vector<unsigned char> advance_slotted(serial::SlottedDataHeader &header);
  bool is_fixed_block_insertable(std::vector<unsigned char> &block_bytes);
  bool is_slotted_block_insertable(std::vector<unsigned char> &block_bytes);

  uint32_t get_insertable_page(uint32_t block_id, uint32_t reg_size);

  uint32_t create_page(serial::TableMetadata &table_metadata);

  uint32_t add_new_page_to_table(serial::TableMetadata &table_metadata);

  std::vector<size_t> get_used_pages(serial::TableMetadata &table_metadata);

  // =====
  // Operaciones de registros
  // =====

  // Transforma un registro completo a bytes
  // @notes realiza cast interno de string->SQL_Type
  std::vector<unsigned char> serialize_register(const serial::TableMetadata &table_metadata,
                                                std::vector<std::string> &values);

  // Retornamos todos los valores de un registro
  std::vector<SQL_type> deserialize_register(const serial::TableMetadata &table_metadata,
                                             std::vector<unsigned char> &register_bytes);

  std::vector<unsigned char> get_ith_register_bytes(
      const serial::TableMetadata &table_metadata,
      const serial::PageHeader &page_header,
      const serial::FixedDataHeader &fixed_data_header,
      const std::vector<unsigned char> &page_bytes,
      size_t ith_reg);

  std::vector<unsigned char> get_ith_register_bytes(
      const serial::TableMetadata &table_metadata,
      const serial::PageHeader &page_header,
      const serial::SlottedDataHeader &slotted_data_header,
      const std::vector<unsigned char> &page_bytes,
      size_t ith_reg);

  void show_table_metadata(std::string &table_name);

  void translate();
  std::string translate_data_page(serial::TableMetadata &table_metadata, size_t page_id);
  std::string translate_data_page(serial::TableMetadata &table_metadata,
                                  std::vector<unsigned char> &page_bytes, size_t page_id);

  std::vector<std::string> translate_fixed_page(
      serial::TableMetadata &table_metadata,
      serial::PageHeader &page_header,
      serial::FixedDataHeader &fixed_data_header,
      std::vector<unsigned char> &page_bytes, uint32_t curr_page_id);

  std::vector<std::string> translate_slotted_page(
      serial::TableMetadata &table_metadata,
      serial::PageHeader &page_header,
      serial::SlottedDataHeader &slotted_data_header,
      std::vector<unsigned char> &page_bytes, uint32_t curr_page_id);

  std::string translate_page_sector();
  std::string translate_table_page(serial::TableMetadata &table_metadata);

  // TRASH
  std::vector<uint32_t> locate_regs_cond(std::string &table_name, std::string &col_name, std::string &condition);
  std::pair<uint32_t, uint32_t> locate_nth_reg(std::string &table_name, size_t nth);
  uint32_t locate_free_page(serial::TableMetadata &table_metadata);
  void show_block(serial::TableMetadata &table_metadata, uint32_t block_id);
  void show_fixed_page(
      serial::TableMetadata &table_metadata,
      serial::PageHeader &page_header,
      serial::FixedDataHeader &fixed_data_header,
      std::vector<unsigned char> &page_bytes, uint32_t curr_page_id);
  std::pair<uint32_t, std::vector<unsigned char>> insert_reg_in_page(serial::TableMetadata &table_metadata, std::string &csv_values);
  std::pair<uint32_t, std::vector<unsigned char>> delete_nth_reg_in_table(serial::TableMetadata &table_metadata, size_t nth);
  uint32_t delete_nth_reg_in_page(std::vector<unsigned char> &page_bytes, size_t nth);

  void run();
  Megatron();
  ~Megatron();
};
