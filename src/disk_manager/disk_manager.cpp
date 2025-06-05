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

size_t disk_manager::get_free_logic_sectors_storable(size_t n_sectors) {
  return get_zeros_sequence_bitset(free_space_bitmap, 0, free_space_bitmap.size(), n_sectors);
}

// Busca un bloque libre
uint32_t disk_manager::get_free_block() {
  for (size_t i{}; i < TOTAL_BLOCKS; ++i) {
    if (free_block_map.is_block_free(i))
      return i;
  }
  return TOTAL_BLOCKS;
}

// size_t disk_manager::get_free_block(size_t n_sectors) {
//   return get_zeros_sequence_bitset(free_space_bitmap, 0, free_space_bitmap.size(), n_sectors);
// }

uint32_t disk_manager::reserve_free_block() {
  uint32_t block_id = get_free_block();

  free_block_map.set_block_used(block_id);

  return block_id;
}

void disk_manager::store_fsb() {
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

void disk_manager::load_fsb() {
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

void disk_manager::new_disk(std::string new_disk_name, size_t surfaces,
                            size_t tracks_per_surf, size_t sectors_per_track,
                            size_t sector_size, size_t sectors_per_block) {
  SURFACES = surfaces;
  TRACKS_PER_SURFACE = tracks_per_surf;
  SECTORS_PER_TRACK = sectors_per_track;
  SECTOR_SIZE = sector_size;
  SECTORS_PER_BLOCK = sectors_per_block;
  BLOCK_SIZE = SECTORS_PER_BLOCK * SECTOR_SIZE;
  TRACK_SIZE = SECTORS_PER_TRACK * SECTOR_SIZE;
  SURFACE_SIZE = TRACKS_PER_SURFACE * TRACK_SIZE;
  DISK_CAPACITY = SURFACES * SURFACE_SIZE;
  TOTAL_SECTORS = SECTORS_PER_TRACK * TRACKS_PER_SURFACE * SURFACES,
  TOTAL_BLOCKS = TOTAL_SECTORS / SECTORS_PER_BLOCK;

  NULL_BLOCK = TOTAL_BLOCKS;

  disk_name = new_disk_name;
  disk_name_txt = new_disk_name + "_txt";

  create_disk_structure(0);
  create_disk_structure(1);

  size_t fsb_bytes_used = (TOTAL_SECTORS + CHAR_BIT - 1) / CHAR_BIT;
  // fbm_bytes_used = (TOTAL_BLOCKS + CHAR_BIT - 1) / CHAR_BIT;

  // Cuantos sectores ocuparia un fsb
  // n_sectors = (fsb_bytes_used + SECTOR_SIZE - 1) / SECTOR_SIZE;

  // Escritura de metadada de disco en sector 0
  serial::Sector_0 sector0;
  sector0.surfaces = SURFACES;
  sector0.tracks_per_surf = TRACKS_PER_SURFACE;
  sector0.sectors_per_track = SECTORS_PER_TRACK;
  sector0.sector_size = SECTOR_SIZE;
  sector0.sectors_per_block = SECTORS_PER_BLOCK;

  free_space_bitmap.resize(TOTAL_SECTORS);

  auto bytes = serial::serialize_sector0(sector0);
  write_sector(bytes, 0);

  //  Caso se guarde bitmap en disco mismo
  //   free_space_bitmap.set(0, n_sectors, true);

  // Bitmap sectores libres

  // free_space_bitmap.set(0, 1, true);

  store_fsb();

  create_free_block_map();
}

void disk_manager::load_disk(std::string load_disk_name) {
  // Caso se haga un switch a otro disco, se tiene que guardar su FSB
  // TODO: check si de verdad valida que se tenia cargado un disco valido
  if (DISK_CAPACITY > 0 && TOTAL_SECTORS > 0 && free_space_bitmap.size() == TOTAL_SECTORS)
    store_fsb();

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
  // read_block(bytes, sector0_size, 0);

  std::ifstream sect0_file(sector0_path, std::ios::binary);

  sect0_file.read(reinterpret_cast<char *>(bytes.data()), sector0_size);
  if (!sect0_file && !sect0_file.eof())
    throw std::runtime_error("Error al leer sector 0");

  serial::Sector_0 sector0 = serial::deserialize_sector0(bytes);

  if (sector0_size != sector0.sector_size)
    throw std::runtime_error("sector 0 corrupto");

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

disk_manager::disk_manager() {
  // // Creamos un disco vacio
  // if (!std::filesystem::exists(DISK_NAME)) {
  //   std::ofstream(DISK_NAME, std::ios::binary).close();
  // }
  //
  // // TODO: Caso cambie tamanio se pierde estructura de disco, ??Alguna forma de preservar data??
  // if (std::filesystem::file_size(DISK_NAME) != DISK_CAP) {
  //   // if (1) {
  //   std::cout << "Disco resized" << std::endl;
  //   std::filesystem::resize_file(DISK_NAME, DISK_CAP);
  //
  //   // free_space_bitmap asociado, el espacio usado por este este se guarda en disco como ocupado
  //   // Se deberia encontrar en primeros bytes del disco(primera supercifie, primeros tracks y sectores)
  //   // TODO: Por ahora se asume como un disco nuevo, de igual forma escribimos dos veces(crear bitmap -> escribirlo -> cargar bitmap)
  //   // Mejorar, usar write para sobreescritura, forzando LBA en 0
  //
  //   // Vacio por defecto
  //   free_space_bitmap.resize(TOTAL_SECTORS);
  //
  //   // Se ocupa el espacio ocupado por bitmap
  //   size_t n_bytes = (free_space_bitmap.size() + CHAR_BIT - 1) / CHAR_BIT,
  //          n_sectors = (n_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
  //
  //   free_space_bitmap.set(0, n_sectors, true);
  //
  //   std::vector<unsigned char> bytes(n_bytes);
  //
  //   // Serializacion de bitmap
  //   boost::to_block_range(free_space_bitmap, bytes.begin());
  //
  //   // write_block(bytes);
  //
  //   disk_file.open(DISK_NAME, std::ios::binary | std::ios::in | std::ios::out);
  //   disk_file.write(reinterpret_cast<const char *>(bytes.data()), n_bytes);
  //   // store_fsb();
  //
  //   disk_file.close();
  // }
  //
  // disk_file.open(DISK_NAME, std::ios::binary | std::ios::in | std::ios::out);
  // written_bytes_file.open("written_bytes.txt");
  // read_bytes_file.open("read_bytes.txt");
  //
  // // Carga de free_space_bitmap
  // load_fsb();
  //
  // if (!disk_file.is_open())
  //   std::cout << "Error al abrir archivo disco" << std::endl;
}

disk_manager::~disk_manager() {
  // Se guarda free_space_bitmap
  store_fsb();
}
