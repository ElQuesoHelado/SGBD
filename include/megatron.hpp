#pragma once

#include "bptree/bptree.hpp"
#include "buffer/buffer_manager.hpp"
#include "comparison.hpp"
#include "disk_manager.hpp"
#include "hash/bucket.hpp"
#include "hash/directory.hpp"
#include "hash/hasher.hpp"
#include "result_set.hpp"
#include "serial/fixed_data.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include "types/types.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class Megatron {
  std::unique_ptr<DiskManager> disk_manager{};
  std::unique_ptr<BufferManager> buffer_manager{};

  static inline std::atomic<bool> global_shutdown{false};

  size_t n_sectors_in_block;

  // Tabla catalog tiene formato: (table_id, col_index, hash_type:0, root_block_id)
  std::string catalog_name = "catalog";

  //====
  //  Funciones para llenado de todo campo de headers/metadata,
  //  internamente se realizan conversiones y algunos calculos
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
  void init_fixed_data_header(size_t reg_size, serial::FixedDataHeader &fixed_data_header);
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

  void select_print(std::string &table_name,
                    Comparator &comparator, int max_pages_loaded = -1);

  void select_print(serial::TableMetadata &table_metadata,
                    Comparator &comparator, int max_pages_loaded = -1);

  ResultSet select(std::string &table_name,
                   Comparator &comparator, int max_pages_loaded = -1);
  ResultSet select(serial::TableMetadata &table_metadata,
                   Comparator &comparator, int max_pages_loaded = -1);
  ResultSet select_from_page(serial::TableMetadata &table_metadata,
                             uint32_t select_page_id, Comparator &comparator);
  ResultSet select_from_fixed_page(serial::TableMetadata &table_metadata,
                                   uint32_t select_page_id, Comparator &comparator);
  ResultSet select_from_slotted_page(serial::TableMetadata &table_metadata,
                                     uint32_t select_page_id, Comparator &comparator);

  ResultSet select_nth_reg(std::string &table_name, size_t nth);
  ResultSet select_nth_reg(serial::TableMetadata &table_metadata, size_t nth);
  ResultSet select_nth_from_page(serial::TableMetadata &table_metadata,
                                 uint32_t delete_page_id, size_t nth);
  ResultSet select_nth_from_fixed_page(serial::TableMetadata &table_metadata,
                                       uint32_t delete_page_id, size_t nth);
  ResultSet select_nth_from_slotted_page(
      serial::TableMetadata &table_metadata,
      uint32_t delete_page_id, size_t nth);

  ResultSet delete_condition(std::string &table_name, Comparator &comparator);
  ResultSet delete_condition(serial::TableMetadata &table_metadata,
                             Comparator &comparator);
  ResultSet delete_from_page(serial::TableMetadata &table_metadata,
                             uint32_t delete_page_id, Comparator &comparator);
  // ResultSet delete_from_page(serial::TableMetadata &table_metadata,
  //                            uint32_t delete_page_id, size_t col_index,
  //                            SQL_type &cond_val);
  ResultSet delete_from_fixed_page(serial::TableMetadata &table_metadata,
                                   uint32_t delete_page_id, Comparator &comparator);
  ResultSet delete_from_slotted_page(serial::TableMetadata &table_metadata,
                                     uint32_t delete_page_id, Comparator &comparator);
  ResultSet delete_nth_reg(std::string &table_name, size_t nth);
  ResultSet delete_nth_reg(serial::TableMetadata &table_metadata, size_t nth);
  ResultSet delete_nth_from_page(serial::TableMetadata &table_metadata,
                                 uint32_t delete_page_id, size_t nth);
  ResultSet delete_nth_from_fixed_page(serial::TableMetadata &table_metadata,
                                       uint32_t delete_page_id, size_t nth);
  ResultSet delete_nth_from_slotted_page(
      serial::TableMetadata &table_metadata,
      uint32_t delete_page_id, size_t nth);

  ResultSet insert(std::string table_name, std::vector<std::string> &values);
  ResultSet insert(serial::TableMetadata &table_metadata, std::vector<std::string> &values);

  ResultSet insert_into_page(serial::TableMetadata &table_metadata,
                             uint32_t insert_page_id,
                             std::vector<unsigned char> &register_bytes);
  // Preprocesa registro
  ResultSet insert_into_page(serial::TableMetadata &table_metadata,
                             uint32_t insert_page_id,
                             std::vector<std::string> &reg_values);

  // Se entiende que pagina tiene capacidad suficiente para registro/+slot
  size_t insert_into_fixed_page(uint32_t insert_page_id, std::vector<unsigned char> &register_bytes);
  size_t insert_into_slotted_page(uint32_t insert_page_id, std::vector<unsigned char> &register_bytes);

  ResultSet update_condition(std::string &table_name,
                             Comparator &comparator,
                             std::string &upd_col_name,
                             std::string &upd_col_value);
  ResultSet update_condition(serial::TableMetadata &table_metadata,
                             Comparator &comparator,
                             std::string &upd_col_name,
                             std::string &upd_col_value);

  // Realiza validaciones de input
  ResultSet update_from_page(serial::TableMetadata &table_metadata,
                             uint32_t update_page_id,
                             Comparator &comparator,
                             std::string &upd_col_name,
                             std::string &upd_col_value);

  ResultSet update_from_page(serial::TableMetadata &table_metadata,
                             uint32_t update_page_id,
                             Comparator &comparator,
                             size_t upd_col_index, SQL_type_ &upd_value);

  ResultSet update_from_fixed_page(serial::TableMetadata &table_metadata,
                                   uint32_t update_page_id,
                                   Comparator &comparator,
                                   size_t upd_col_index, SQL_type_ &upd_value);

  ResultSet update_from_slotted_page(serial::TableMetadata &table_metadata,
                                     uint32_t update_page_id,
                                     Comparator &comparator,
                                     size_t upd_col_index, SQL_type_ &upd_value);

  ResultSet update_nth_reg(std::string &table_name, size_t nth,
                           std::string &upd_col_name,
                           std::string &upd_col_value);

  ResultSet update_nth_reg(serial::TableMetadata &table_metadata, size_t nth,
                           std::string &upd_col_name,
                           std::string &upd_col_value);

  ResultSet update_nth_from_page(serial::TableMetadata &table_metadata,
                                 uint32_t update_page_id, size_t nth,
                                 std::string &upd_col_name,
                                 std::string &upd_col_value);

  ResultSet update_nth_from_page(serial::TableMetadata &table_metadata,
                                 uint32_t update_page_id, size_t nth,
                                 size_t upd_col_index, SQL_type_ &upd_value);

  ResultSet update_nth_from_fixed_page(serial::TableMetadata &table_metadata,
                                       uint32_t update_page_id, size_t nth,
                                       size_t upd_col_index,
                                       SQL_type_ &upd_value);

  ResultSet update_nth_from_slotted_page(serial::TableMetadata &table_metadata,
                                         uint32_t update_page_id, size_t nth,
                                         size_t upd_col_index,
                                         SQL_type_ &upd_value);

  void select_save(std::string table_name, std::string col_name,
                   std::string condition, std::string new_table_name);

  void find_nth_reg(std::string &table_name, size_t nth);

  /*
   * @brief Creacion y escritura de nueva tabla descrita en csv
   * @note CSV con formato:
   * NombreRelacion,,,...,
   * camp1#tipo1,camp2#tipo2,...
   * dato1,dato2,...
   */
  void load_CSV(std::string csv_path, std::string table_name, size_t n_regs = 0);

  // =============================
  // Hashes
  // =============================
  bool is_column_hashed(std::string &table_name, std::string &col_name);
  bool is_column_hashed(serial::TableMetadata &table_metadata, std::string &col_name);
  bool is_column_hashed(serial::TableMetadata &table_metadata, size_t col_index);

  std::vector<std::pair<uint32_t, uint32_t>>
  get_hashed_columns(std::string &table_name);
  std::vector<std::pair<uint32_t, uint32_t>>
  get_hashed_columns(serial::TableMetadata &table_metadata);

  void add_hash_to_table(std::string &table_name, std::string &col_name);
  void add_hash_to_table(serial::TableMetadata &table_metadata, std::string &col_name);
  void add_hash_to_table(serial::TableMetadata &table_metadata, size_t col_index);

  size_t create_dir_page();
  size_t create_bucket_page();

  // void insert_hashed(serial::TableMetadata &table_metadata,
  //                    size_t col_index, size_t page_id, size_t pos,
  //                    std::vector<unsigned char> &register_bytes);
  bool insert_hashed(serial::TableMetadata table_metadata, size_t hashed_col,
                     SQL_type_ &key, size_t inserted_page, size_t inserted_slot);

  uint32_t get_root_page_id(serial::TableMetadata &table_metadata, size_t col_index);

  ResultSet get_register_on_key_match(serial::TableMetadata &table_metadata,
                                      RegPtr &reg_ptr, size_t hashed_col,
                                      SQL_type_ &key);

  ResultSet find(serial::TableMetadata table_metadata,
                 size_t hashed_col, SQL_type_ &key);

  ResultSet delete_hashed(serial::TableMetadata table_metadata,
                          size_t hashed_col, SQL_type_ &key);

  size_t find_free_reg_ptr_pos(Bucket &bucket);

  DirectoryPage load_directory_page(size_t page_id);

  // =============================
  // Indices
  // =============================
  bool is_column_indexed(std::string &table_name, std::string &col_name);
  bool is_column_indexed(serial::TableMetadata &table_metadata,
                         std::string &col_name);
  bool is_column_indexed(serial::TableMetadata &table_metadata, size_t col_index);

  std::pair<uint32_t, uint32_t>
  get_indexed_column(serial::TableMetadata &table_metadata, size_t col_index);

  std::vector<std::pair<uint32_t, uint32_t>>
  get_indexed_columns(std::string &table_name);

  std::vector<std::pair<uint32_t, uint32_t>>
  get_indexed_columns(serial::TableMetadata &table_metadata);

  void add_index_to_table(std::string &table_name, std::string &col_name);
  void add_index_to_table(serial::TableMetadata &table_metadata, std::string &col_name);
  void add_index_to_table(serial::TableMetadata &table_metadata, size_t col_index);

  size_t calculate_btree_order(size_t key_size);

  // Helprs
  // void print_relation(Relation &relation);
  size_t char_size(std::string type);

  float table_size(std::string name);

  // Concatenacion de operaciones en varias columnas
  // Se pasa tuplas nombre_columna, operacion, valor | AND/OR
  // @Notes operacion: <, >, <=, >=, ==
  Comparator generate_comparator(serial::TableMetadata &table_metadata,
                                 std::vector<std::tuple<std::string,
                                                        std::string,
                                                        std::string>>
                                     &comparisons);

  //(index columna, operador, SQL_type con valor a comparar)
  Comparator generate_comparator(serial::TableMetadata &table_metadata,
                                 std::vector<std::tuple<size_t,
                                                        std::string,
                                                        SQL_type_>>
                                     &comparisons);

  /*
   * Une buffers de sectores individuales, produce un bloque continuo
   */
  std::vector<unsigned char> merge_sectors_to_block(std::vector<std::vector<unsigned char>> &sectors_bytes);
  std::vector<std::vector<unsigned char>> split_block_to_sectors(std::vector<unsigned char> &&block_bytes);

  // =============================
  // Operaciones directas en disco
  // =============================

  void new_disk(std::string disk_name, size_t surfaces, size_t tracks, size_t sectors,
                size_t bytes, size_t sectors_block, size_t n_frames, bool is_clock);
  void load_disk(std::string disk_name, size_t n_frames, bool is_clock);

  void set_buffer_manager_frames();

  // Determina si un bloque tiene capacidad de insertar al menos un registro
  // el tamanio del registro esta dado internamente en el header de cada sector
  // @return -1 si ningun sector es disponible, sino de [0, n_sectors_in_block)
  // int is_fixed_block_insertable(const serial::Block &block);
  // int is_slotted_block_insertable(const serial::Block &block, size_t reg_size);

  /*
   * Se busca tabla en sector 1 del disco
   */
  bool search_table(std::string name, serial::TableMetadata &table_metadata);
  bool search_table(size_t table_id, serial::TableMetadata &table_metadata);

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

  size_t get_column_index(serial::TableMetadata &table_metadata, std::string &col_name);

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
  void ui_update_nth_reg();
  void ui_delete_data();
  void ui_delete_nth();
  void ui_load_csv();
  void ui_load_n_regs_csv();
  void ui_show_page();
  void ui_show_table_metadata();
  void ui_add_hash_to_table();
  void ui_add_index_to_table();
  void ui_show_threads_from_board();
  void ui_show_posts_from_thread();
  void ui_delete_thread();
  void ui_show_post_media();

  Comparator ui_generate_comparator(serial::TableMetadata &table_metadata);
  Comparator ui_generate_comparator(std::string &table_name);

  // Buffer UI
  void ui_interact_buffer_manager();
  void buf_load_page();
  void buf_show_page_content(serial::TableMetadata &table_metadata);
  void buf_select_all(serial::TableMetadata &table_metadata);
  void buf_select_condition(serial::TableMetadata &table_metadata);
  void buf_update_condition(serial::TableMetadata &table_metadata);
  void buf_update_nth(serial::TableMetadata &table_metadata);
  void buf_delete_condition(serial::TableMetadata &table_metadata);
  void buf_delete_nth(serial::TableMetadata &table_metadata);
  void buf_insert_line(serial::TableMetadata &table_metadata);
  void buf_insert_n_csv(serial::TableMetadata &table_metadata);

  // =====
  // Operaciones de buscar un bloque
  // =====

  // En base a un header se avanza al siguiente bloque
  std::vector<unsigned char> advance_fixed(serial::FixedDataHeader &header);
  std::vector<unsigned char> advance_slotted(serial::SlottedDataHeader &header);
  bool is_fixed_block_insertable(std::vector<unsigned char> &block_bytes);
  bool is_slotted_block_insertable(std::vector<unsigned char> &block_bytes);

  // Itera a travez de heapfile hasta encontrar pagina libre
  uint32_t get_insertable_page_id(uint32_t first_page_id, uint32_t reg_size);

  uint32_t create_page(serial::TableMetadata &table_metadata);
  uint32_t create_fixed_page(size_t reg_size);

  uint32_t add_new_page_to_table(serial::TableMetadata &table_metadata);

  std::vector<std::pair<size_t, size_t>> get_used_pages(serial::TableMetadata &table_metadata);

  // =====
  // Operaciones de registros
  // =====

  // Transforma un registro completo a bytes
  // @notes realiza cast interno de string->SQL_Type
  std::vector<unsigned char> serialize_register(const serial::TableMetadata &table_metadata,
                                                std::vector<std::string> &values);
  std::vector<unsigned char> serialize_register(const serial::TableMetadata &table_metadata,
                                                std::vector<SQL_type_> &values);

  // Retornamos todos los valores de un registro
  std::vector<SQL_type_> deserialize_register(const serial::TableMetadata &table_metadata,
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

  std::pair<std::vector<uint32_t>, std::vector<uint32_t>> translate();
  std::string translate_data_page(serial::TableMetadata &table_metadata, size_t page_id);
  std::string translate_data_page(serial::TableMetadata &table_metadata,
                                  std::vector<unsigned char> &page_bytes, size_t page_id);
  std::string translate_data_page_no_write(serial::TableMetadata &table_metadata,
                                           std::vector<unsigned char> &page_bytes, size_t page_id);

  std::vector<uint32_t> translate_bptree_node_page(
      serial::TableMetadata &table_metadata, BPTree &tree,
      uint32_t node_id);

  void translate_hash_directory(
      serial::TableMetadata &table_metadata, DirectoryPage &dir);

  void translate_bucket(
      serial::TableMetadata &table_metadata, Bucket &bucket,
      Hasher &hasher,
      std::vector<unsigned char> &page_bytes,
      uint32_t curr_page_id, uint16_t capacity);

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

  void run();
  Megatron();
  ~Megatron();
};
