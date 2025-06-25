#include "disk_manager.hpp"
#include "serial/sector0.hpp"
#include <algorithm>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>

// size_t disk_manager::get_free_logic_sectors_dbms(size_t n_sectors) {
//   return get_zeros_sequence_bitset(free_space_bitmap, FIRST_DBMS_LBA, FIRST_STORABLE_LBA, n_sectors);
// }

size_t DiskManager::get_free_logic_sectors_storable(size_t n_sectors) {
  return get_zeros_sequence_bitset(free_space_bitmap, 0, free_space_bitmap.size(), n_sectors);
}

// Busca un bloque libre
// @note NO reserva
uint32_t DiskManager::get_free_block() {
  for (size_t i{}; i < TOTAL_BLOCKS; ++i) {
    if (free_block_map.is_block_free(i))
      return i;
  }
  return TOTAL_BLOCKS;
}

// size_t disk_manager::get_free_block(size_t n_sectors) {
//   return get_zeros_sequence_bitset(free_space_bitmap, 0, free_space_bitmap.size(), n_sectors);
// }

uint32_t DiskManager::reserve_free_block() {
  uint32_t block_id = get_free_block();

  set_block_used(block_id);

  return block_id;
}

void DiskManager::set_block_used(uint32_t block_id) {
  if (block_id >= NULL_BLOCK)
    throw std::runtime_error("block_id fuera de rango");

  for (auto sector : free_block_map.blocks[block_id].second) {
    free_space_bitmap.set(sector, 1, true);
  }

  free_block_map.blocks[block_id].first = true;
}

void DiskManager::store_fsb() {
  size_t n_bytes = (TOTAL_SECTORS + CHAR_BIT - 1) / CHAR_BIT;
  std::vector<unsigned char> bytes(n_bytes);

  // Serializacion de bitmap
  boost::to_block_range(free_space_bitmap, bytes.begin());

  // Guardar en archivo
  std::ofstream out_file(disk_name + "/free_space_bitmap", std::ios::binary);
  if (!out_file)
    throw std::runtime_error("No se pudo abrir free_space_bitmap para escritura: " + disk_name + "/free_space_bitmap");

  out_file.write(reinterpret_cast<const char *>(bytes.data()), n_bytes);
  if (!out_file)
    throw std::runtime_error("Error al escribir free_space_bitmap: " + disk_name + "/free_space_bitmap");

  // write_block(bytes, 0);
}

void DiskManager::load_fsb() {
  size_t n_bytes = (TOTAL_SECTORS + CHAR_BIT - 1) / CHAR_BIT;
  std::vector<unsigned char> bytes(n_bytes);

  // Leer del archivo
  std::ifstream in_file(disk_name + "/free_space_bitmap", std::ios::binary);
  if (!in_file) {
    // Caso no hay bitmap,inicializar bitmap vacío, se espera guardar luego
    // free_space_bitmap.clear();
    // free_space_bitmap.resize(TOTAL_SECTORS, false);
    throw std::runtime_error(disk_name + "/free_space_bitmap no existe");
    return;
  }

  in_file.read(reinterpret_cast<char *>(bytes.data()), n_bytes);
  if (!in_file && !in_file.eof())
    throw std::runtime_error("Error al leer el archivo bitmap: " + disk_name + "/free_space_bitmap");

  // Serializacion de bitmap
  free_space_bitmap.clear();
  free_space_bitmap.resize(TOTAL_SECTORS);
  boost::from_block_range(bytes.begin(), bytes.end(), free_space_bitmap);
}

void DiskManager::new_disk(std::string new_disk_name, size_t surfaces,
                           size_t tracks_per_surf, size_t sectors_per_track,
                           size_t sector_size, size_t sectors_per_block) {
  DiskManager::create_disk_structure(0, new_disk_name, surfaces, tracks_per_surf, sectors_per_track,
                                     sector_size);
  DiskManager::create_disk_structure(1, new_disk_name, surfaces, tracks_per_surf, sectors_per_track,
                                     sector_size);

  // Crear sector 0
  {
    serial::Sector_0 sector0;
    sector0.surfaces = surfaces;
    sector0.tracks_per_surf = tracks_per_surf;
    sector0.sectors_per_track = sectors_per_track;
    sector0.sector_size = sector_size;
    sector0.sectors_per_block = sectors_per_block;

    auto bytes = serial::serialize_sector0(sector0);
    bytes.resize(sector_size);

    std::ofstream out_file(new_disk_name + "/superficie 0/pista 0/sector 0", std::ios::binary);
    if (!out_file)
      throw std::runtime_error("No se pudo abrir sector0 para escritura: ");

    out_file.write(reinterpret_cast<const char *>(bytes.data()), sector_size);
    if (!out_file)
      throw std::runtime_error("Error al escribir sector0: ");
  }

  // Guardar bitmap
  {
    size_t total_sectors = sectors_per_track * tracks_per_surf * surfaces,
           fsb_bytes_used = (total_sectors + CHAR_BIT - 1) / CHAR_BIT;

    std::ofstream out_file(new_disk_name + "/free_space_bitmap", std::ios::binary);

    if (!out_file)
      throw std::runtime_error(
          "No se pudo abrir free_space_bitmap para escritura: " + new_disk_name + "/free_space_bitmap");

    std::vector<char> zeros(fsb_bytes_used, 0);

    out_file.write(zeros.data(), fsb_bytes_used);
    out_file.close();

    if (!out_file)
      throw std::runtime_error("Error al escribir free_space_bitmap: " + new_disk_name + "/free_space_bitmap");
  }
}

void DiskManager::load_disk(std::string load_disk_name) {
  // Caso se haga un switch a otro disco, se tiene que guardar su FSB
  // TODO: check si de verdad valida que se tenia cargado un disco valido
  // if (DISK_CAPACITY > 0 && TOTAL_SECTORS > 0 && free_space_bitmap.size() == TOTAL_SECTORS)
  //   store_fsb();

  namespace fs = std::filesystem;
  std::string sector0_path = load_disk_name + "/superficie 0/pista 0/sector 0",
              sector0_path_txt = load_disk_name + "_txt" + "/superficie 0/pista 0/sector 0.txt";

  if (!fs::exists(sector0_path) || !fs::is_regular_file(sector0_path) ||
      !fs::exists(sector0_path_txt) || !fs::is_regular_file(sector0_path_txt)) {
    throw std::runtime_error("Disco: " + disk_name + " no existe/sector 0 invalido");
  }

  disk_name = load_disk_name;
  disk_name_txt = load_disk_name + "_txt";

  // Necesitamos leer el tamanio del primer sector desde filesystem
  size_t sector0_size = fs::file_size(sector0_path);
  std::vector<unsigned char> bytes(sector0_size);

  std::ifstream sect0_file(sector0_path, std::ios::binary);

  sect0_file.read(reinterpret_cast<char *>(bytes.data()), sector0_size);
  if (!sect0_file && !sect0_file.eof())
    throw std::runtime_error("Error al leer sector 0");

  serial::Sector_0 sector0 = serial::deserialize_sector0(bytes);

  if (sector0_size != sector0.sector_size)
    throw std::runtime_error("Sector 0 corrupto");

  SURFACES = sector0.surfaces;
  TRACKS_PER_SURFACE = sector0.tracks_per_surf;
  SECTORS_PER_TRACK = sector0.sectors_per_track;
  SECTOR_SIZE = sector0.sector_size;
  SECTORS_PER_BLOCK = sector0.sectors_per_block;
  BLOCK_SIZE = SECTORS_PER_BLOCK * SECTOR_SIZE;
  TRACK_SIZE = SECTORS_PER_TRACK * SECTOR_SIZE;
  SURFACE_SIZE = TRACKS_PER_SURFACE * TRACK_SIZE;
  DISK_CAPACITY = SURFACES * SURFACE_SIZE;
  TOTAL_SECTORS = SECTORS_PER_TRACK * TRACKS_PER_SURFACE * SURFACES,
  TOTAL_BLOCKS = TOTAL_SECTORS / SECTORS_PER_BLOCK;

  NULL_BLOCK = TOTAL_BLOCKS;

  load_fsb();
  create_free_block_map();
}

void DiskManager::persist() {
  store_fsb();
}

// Crea y carga disco
DiskManager::DiskManager(std::string new_disk_name, size_t surfaces,
                         size_t tracks_per_surf, size_t sectors_per_track,
                         size_t sector_size, size_t sectors_per_block) {
  new_disk(new_disk_name, surfaces, tracks_per_surf, sectors_per_track,
           sector_size, sectors_per_block);
  load_disk(new_disk_name);
}

// Carga disco basado en estructura de carpetas
DiskManager::DiskManager(std::string load_disk_name) {
  load_disk(load_disk_name);
}

DiskManager::~DiskManager() {
  // Se guarda free_space_bitmap
  persist();
}
