#pragma once

#include "block_map.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <vector>
// #include <sector>

/*
 * Representa toda operacion relacionada a disco
 * Toda operacion se realiza con el modelo Cylinder Head(superficie) Sector
 */
class DiskManager {

  size_t cur_head{}, cur_cylinder{}, cur_sector{};

  std::fstream curr_sector_file{}, curr_sector_txt_file{};

  // ===========
  // Manejo disco
  // ===========

  size_t get_zeros_sequence_bitset(boost::dynamic_bitset<unsigned char> &set, size_t begin, size_t end, size_t n);
  void store_fsb();
  void load_fsb();

public:
  size_t
      SURFACES{},
      TRACKS_PER_SURFACE{},
      SECTORS_PER_TRACK{},
      SECTOR_SIZE{},
      SECTORS_PER_BLOCK{},
      BLOCK_SIZE{}, // Derivados
      TRACK_SIZE{},
      SURFACE_SIZE{},
      DISK_CAPACITY{},
      TOTAL_SECTORS{},
      TOTAL_BLOCKS{}; // Truncado

  uint32_t NULL_BLOCK;

  std::string disk_name, disk_name_txt;

  FreeBlockMap free_block_map;

  void create_disk_structure(bool bin);

  // Representacion LBA(Logical Block Addresing)
  //  Cargado en memoria para busquedas rapidas, luego se guarda en disco
  //  TODO: Por ahora se trabaja su guardado de forma diferente(directa), sin usar sectores, bloques, etc
  //  Concuerda con el tamanio de 8 tracks

  // ===========
  // Operaciones de cabecera
  // ===========

  /*
   * Se itera por todo el disco de forma vertical para agrupar bloques,
   * caso al menos un sector este usado, se asume bloque como ocupado
   */
  void create_free_block_map();

  boost::dynamic_bitset<unsigned char> free_space_bitmap;

  // size_t get_free_logic_sectors_dbms(size_t n_sectors);

  size_t get_free_logic_sectors_storable(size_t n_sectors);

  uint32_t get_free_block();

  /*
   * Se marcan sectores como ocupados y los devuelve en un bloque
   * TODO: es lineal por ahora
   */
  uint32_t reserve_free_block();

  void seek(size_t cilinder, size_t head, size_t sector);
  // void rotate(size_t sect);
  // void rotate();

  void logic_sector_move_CHS(size_t logic_sector);

  size_t logic_sector_to_byte(size_t logic_sector);

  // ===========
  // Write sectors
  // ===========

  /*
   * Write de mas bajo nivel, marca escritura en bitmap y mueve cabecera
   * Realiza el movimiento de cabecera y escribe un bloque
   */
  void seek_write_sector(unsigned char *sector, size_t logic_sector);

  // Siempre escribe en sectores libres
  size_t write_sector(std::vector<unsigned char> &bytes);

  // Para sobreescritura
  void write_sector(std::vector<unsigned char> &bytes, size_t first_logic_sector);

  // ===========
  // Writes Blocks
  // ===========

  /*
   * Se escribe sectores correspondientes a sectores de un bloque
   */
  void write_block(std::vector<unsigned char> &block_bytes, uint32_t block_id);

  // Dependiendo de bloques libres
  uint32_t write_block(std::vector<unsigned char> &block_bytes);

  // Por ahora es correcto, en deletes cambia
  void write_sector_txt(std::string str, size_t logic_sector);
  void write_block_txt(std::string str, uint32_t block_id);

  // ===========
  // Read Sectors
  // ===========

  // Realiza el movimiento de cabecera para leer
  void seek_read_sector(unsigned char *sector, size_t logic_sector);

  /*
   * Lee un bloque, el bloque resultado se limpia
   */
  void read_sector(std::vector<unsigned char> &bytes, size_t first_logic_sector);

  // ===========
  // Read Blocks
  // ===========

  void read_block(std::vector<unsigned char> &block, uint32_t block_id);

  // ===========
  // Disks
  // ===========

  /*
   * Crea disco con cierto nombre, implica crear estructura completa, free_space_bitmap
   * y guardar datos en sector 0(superficies, )
   */
  void new_disk(std::string new_disk_name, size_t surfaces,
                size_t tracks_per_surf, size_t sectors_per_track,
                size_t sector_size, size_t sectors_per_block);

  /*
   * Carga y calcula specs disco basado en carpetas/files
   * Se carga free_space_bitmap
   */
  void load_disk(std::string load_disk_name);

  std::string logic_sector_to_CHS(size_t logic_sector);

  std::vector<unsigned char> merge_sectors_to_block(std::vector<std::vector<unsigned char>> &sectors_bytes);
  std::vector<std::vector<unsigned char>> split_block_to_sectors(std::vector<unsigned char> &block_bytes);

  void clear_blocks_folder();

  size_t calculate_free_space();

  /*
   * Como tal carga el disco actual, caso exista
   * TODO: disco en estado invalido
   */
  DiskManager();

  ~DiskManager();
};
