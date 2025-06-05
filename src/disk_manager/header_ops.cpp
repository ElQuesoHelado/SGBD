#include "disk_manager.hpp"
#include <filesystem>
#include <iostream>

// Posiciona head en track, superficie no cuesta
// TODO: como se simula disco, necesariamente deberia trabajar desde posicion actual
// Puede moverse tanto adelante o atras
void disk_manager::seek(size_t cilinder, size_t head, size_t sector) {
  if (head > SURFACES || cilinder > TRACKS_PER_SURFACE || sector > SECTORS_PER_TRACK)
    return;

  std::string path = disk_name + "/superficie " + std::to_string(head) +
                     "/pista " + std::to_string(cilinder) +
                     "/sector " + std::to_string(sector);
  ;

  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("El sector especificado no existe: " + path);
  }

  // if (disk_file.fail())
  //   std::cerr << "ANTES SEEK Error: Fallo en operación (formato incorrecto, etc.)\n";

  cur_head = head;
  cur_cylinder = cilinder;
  cur_sector = sector;

  curr_sector_file.close();
  curr_sector_file.open(path, std::ios::binary | std::ios::in | std::ios::out);
  curr_sector_file.seekp(0);

  if (!curr_sector_file.is_open()) {
    std::cerr << "No se pudo abrir sector: " << path << "\n";
    return;
  }
  // curr_sector_file.seekg(SURFACE_SIZE * (head) + TRACK_SIZE * (cilinder) + SECTOR_SIZE * sector, std::ios::beg);
  // curr_sector_file.seekp(SURFACE_SIZE * (head) + TRACK_SIZE * (cilinder) + SECTOR_SIZE * sector, std::ios::beg);
  //
  // std::cout << "Pos ptrFile " << disk_file.tellp() << std::endl;

  // if (curr_sector_file.fail())
  //   std::cerr << "Error: Fallo en operación (formato incorrecto, etc.)\n";

  // if(disk_file.rdstate() == std::ios::)
  //   std::cout<<"Fuera de archivo"<<std::endl;

  // std::cout << SURFACE_SIZE * (surfc - 1) << "  " << TRACK_SIZE * (track - 1) << std::endl;
  // std::cout << disk_file.tellg() << std::endl;
  // 23622381568
}

// Rotacion a un sector arbitrario, deja cabeza justo encima del primer byte del sector, cabeza debe estar en sector 1
// TODO: Rota en solo una direccion, se deberia considerar posicion actual, actualmente depende en una llamada previa de seek
// void disk_manager::rotate(size_t sect) {
//   if (sect > SECTORS)
//     return;
//
//   disk_file.seekg(SECTOR_SIZE * (sect), std::ios::cur);
//   disk_file.seekp(SECTOR_SIZE * (sect), std::ios::cur);
//
//   cur_sect = sect;
//
//   // std::cout << disk_file.tellg() << std::endl;
// }

// Realiza un solo rotate al sector siguiente en track
// void disk_manager::rotate() {
//   disk_file.seekg(SECTOR_SIZE, std::ios::cur);
//   disk_file.seekp(SECTOR_SIZE, std::ios::cur);
//
//   if (cur_sect == SECTORS)
//     cur_sect = 1;
//   else
//     ++cur_sect;
// }

// Posiciona cabeza en lugar exacto basado en LBA
void disk_manager::logic_sector_move_CHS(size_t logic_sector) {
  size_t cilinder = logic_sector / (SURFACES * SECTORS_PER_TRACK),
         head = (logic_sector / SECTORS_PER_TRACK) % SURFACES,
         sector = (logic_sector % SECTORS_PER_TRACK);

  seek(cilinder, head, sector);
  // rotate(sector);
}
