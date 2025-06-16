#include "disk_manager.hpp"
#include <iostream>
#include <string>

// Solo escribe un sector en disco
// Asume que ha sido posicionado previamente en un lugar valido
void DiskManager::seek_write_sector(unsigned char *sector, size_t logic_sector) {
  logic_sector_move_CHS(logic_sector);
  // written_bytes_file << logic_sector << "   " << disk_file.tellp() << "    "
  //                    << cur_cylinder << " " << cur_head << " " << cur_sector << std::endl;

  // std::cout << "WRITE BYTE: " << disk_file.tellp() << std::endl;
  // curr_sector_file.write(reinterpret_cast<const char *>(sector), SECTOR_SIZE);
  curr_sector_file.write(reinterpret_cast<const char *>(sector), SECTOR_SIZE);
  curr_sector_file.flush();

  free_space_bitmap.set(logic_sector);

  if (!curr_sector_file.good()) { // Equivalente a checking fail() o bad()
    std::cerr << "Error lógico de escritura\n";
  }
  // rotate();
}

// Escribe un sector
// @note Caso sea menor a size de sector, se rellena, caso mayor, throw
size_t DiskManager::write_sector(std::vector<unsigned char> &bytes) {
  size_t free_sector = get_free_logic_sectors_storable(1);

  write_sector(bytes, free_sector);

  return free_sector;
}

void DiskManager::write_sector(std::vector<unsigned char> &bytes, size_t logic_sector) {
  if (bytes.size() < SECTOR_SIZE)
    bytes.resize(SECTOR_SIZE);

  size_t n_sectors = std::ceil(static_cast<double>(bytes.size()) / SECTOR_SIZE);

  if (n_sectors != 1) {
    throw std::runtime_error("Buffer individual a escribir mas grande que sector");
  }

  seek_write_sector(bytes.data(), logic_sector);
}

void DiskManager::write_sector_txt(std::string str, size_t logic_sector) {
  size_t cilinder = logic_sector / (SURFACES * SECTORS_PER_TRACK),
         head = (logic_sector / SECTORS_PER_TRACK) % SURFACES,
         sector = (logic_sector % SECTORS_PER_TRACK);

  std::string path_txt = disk_name_txt + "/superficie " + std::to_string(head) +
                         "/pista " + std::to_string(cilinder) +
                         "/sector " + std::to_string(sector) + ".txt";

  std::fstream file(path_txt, std::ios::out | std::ios::app);
  if (!file.is_open()) {
    std::cerr << "Error al abrir el archivo." << std::endl;
  }
  file << str << std::endl;

  file.close();
}

void DiskManager::write_block_txt(std::string str, uint32_t block_id) {
  std::string path_txt = "bloques/bloque " + std::to_string(block_id) + ".txt";

  std::fstream file(path_txt, std::ios::out | std::ios::app);
  if (!file.is_open()) {
    std::cerr << "Error al abrir el archivo." << std::endl;
  }
  file << str << std::endl;

  file.close();
}
