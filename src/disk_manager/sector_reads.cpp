#include "disk_manager.hpp"
#include <iostream>

void DiskManager::seek_read_sector(unsigned char *sector, size_t logic_sector) {
  logic_sector_move_CHS(logic_sector);
  curr_sector_file.read(reinterpret_cast<char *>(sector), SECTOR_SIZE);

  if (curr_sector_file.bad()) {
    std::cerr << "Lectura erronea" << std::endl;
  }
}

void DiskManager::read_sector(std::vector<unsigned char> &bytes, size_t first_logic_sector) {
  bytes.clear();
  bytes.resize(SECTOR_SIZE);

  seek_read_sector(bytes.data(), first_logic_sector);
}
