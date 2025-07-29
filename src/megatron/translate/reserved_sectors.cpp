#include "megatron.hpp"
#include "serial/sector0.hpp"
#include "serial/sector1.hpp"

std::vector<uint32_t> Megatron::translate_reserved_sectors() {
  // Se traduce sector0
  std::vector<unsigned char> buffer;
  disk_manager->read_sector(buffer, 0);
  auto sector0 = serial::deserialize_sector0(buffer);

  DiskManager::create_disk_structure(
      0, disk_manager->disk_name,
      sector0.surfaces, sector0.tracks_per_surf,
      sector0.sectors_per_track, sector0.sector_size);

  auto format_str =
      std::format("Superficies: {}, tracks: {}, sectores: {}, tamanio de sector: {},"
                  "sectores por bloque: {}  \n",
                  sector0.surfaces, sector0.tracks_per_surf, sector0.sectors_per_track,
                  sector0.sector_size, sector0.sectors_per_block);

  disk_manager->write_sector_txt(format_str, 0);

  // Sector1
  disk_manager->read_sector(buffer, 1);
  auto sector1 = serial::deserialize_sector1(buffer);

  format_str = "Numero total de tablas: " + std::to_string(sector1.n_tables) + "\n";

  for (size_t i{}; i < sector1.n_tables; ++i)
    format_str += "Tabla #" + std::to_string(i) +
                  " ubicada en bloque: " + std::to_string(sector1.table_block_ids[i]) + "\n";

  disk_manager->write_sector_txt(format_str, 1);

  return std::move(sector1.table_block_ids);
}
