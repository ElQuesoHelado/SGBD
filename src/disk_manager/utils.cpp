#include "block_map.hpp"
#include "disk_manager.hpp"
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>

size_t DiskManager::calculate_free_space() {
  size_t free_space{};
  for (size_t i{}; i < TOTAL_SECTORS; ++i) {
    if (!free_space_bitmap.at(i))
      free_space += SECTOR_SIZE;
  }

  return free_space;
}

size_t DiskManager::logic_sector_to_byte(size_t logic_sector) {
  size_t cilinder = logic_sector / (SURFACES * SECTORS_PER_TRACK),
         head = (logic_sector / SECTORS_PER_TRACK) % SURFACES,
         sector = (logic_sector % SECTORS_PER_TRACK);

  return SURFACE_SIZE * (head) + TRACK_SIZE * (cilinder) + SECTOR_SIZE * sector;
}

std::string DiskManager::logic_sector_to_CHS(size_t logic_sector) {
  size_t cilinder = logic_sector / (SURFACES * SECTORS_PER_TRACK),
         head = (logic_sector / SECTORS_PER_TRACK) % SURFACES,
         sector = (logic_sector % SECTORS_PER_TRACK);

  std::string ret{};
  return ret + "Superficie: " + std::to_string(head) + " Track: " +
         std::to_string(cilinder) + " Sector: " +
         std::to_string(sector);
}

/*
 * Busca secuencia de 0's de size n consecutivos en subrango [begin, end)
 * Devuelve primer indice de secuencia
 */
size_t DiskManager::get_zeros_sequence_bitset(boost::dynamic_bitset<unsigned char> &set,
                                              size_t begin, size_t end, size_t n) {
  size_t seq_start{end}, n_left{n};

  for (size_t i{begin}; i < end; ++i) {
    if (n_left == 0)
      return seq_start;

    if (!set.at(i)) {
      if (seq_start == end)
        seq_start = i;
      --n_left;
    } else {
      seq_start = end;
      n_left = n;
    }
  }
  if (n_left == 0)
    return seq_start;
  return boost::dynamic_bitset<>::npos;
}

void DiskManager::create_disk_structure(bool bin, std::string disk_name_used, size_t surfaces,
                                        size_t tracks_per_surf, size_t sectors_per_track,
                                        size_t sector_size) {
  namespace fs = std::filesystem;

  if (!bin)
    disk_name_used += "_txt";

  // Eliminamos el disco caso se quiera reemplazar(nombre==disco que ya existe)
  if (fs::exists(disk_name_used))
    fs::remove_all(disk_name_used);

  fs::create_directory(disk_name_used);

  // Se borra todo excepto metadatas
  for (const auto &entry : fs::directory_iterator(disk_name_used)) {
    if (entry.is_directory()) {
      std::string name = entry.path().filename().string();
      if (name.find("superficie ") == 0) {
        fs::remove_all(entry.path());
      }
    }
  }

  // Se crea toda la estructura
  for (int s = 0; s < surfaces; ++s) {
    std::string surf_folder = disk_name_used + "/superficie " + std::to_string(s);
    fs::create_directory(surf_folder);

    for (int p = 0; p < tracks_per_surf; ++p) {
      std::string track_folder = surf_folder + "/pista " + std::to_string(p);
      fs::create_directory(track_folder);

      for (int sec = 0; sec < sectors_per_track; ++sec) {
        std::string sector_file = track_folder + "/sector " + std::to_string(sec);

        if (bin) {
          std::ofstream out(sector_file, std::ios::binary | std::ios::trunc);
          std::vector<char> zeros(sector_size, 0);
          out.write(zeros.data(), sector_size);
          out.close();
        } else {
          std::ofstream(sector_file + ".txt").close();
        }
      }
    }
  }
}

/*
 * Cada vez que se realiza un translate, se debe limpiar esta carpeta
 */
void DiskManager::clear_blocks_folder() {
  namespace fs = std::filesystem;
  if (free_block_map.blocks.empty())
    throw std::runtime_error("free_block_map no existe para crear bloques txt");

  std::string block_path = "bloques";

  // Elimina todo directorio bloques
  if (fs::exists(block_path))
    fs::remove_all(block_path);

  fs::create_directory(block_path);

  // Se agrega un bloque.txt a cada bloque del mapa
  for (size_t i{}; i < free_block_map.blocks.size(); ++i) {
    std::ofstream(block_path + "/bloque " + std::to_string(i) + ".txt").close();
  }
}

void DiskManager::create_free_block_map() {
  std::vector<uint32_t> curr_block;
  bool used = false; // 0 es libre

  free_block_map.blocks.clear();

  for (int c = 0; c < TRACKS_PER_SURFACE; ++c) {
    for (int s = 0; s < SECTORS_PER_TRACK; ++s) { // por sectores separados
      for (int h = 0; h < SURFACES; ++h) {
        // Sectores reservados
        if (c == 0 && h == 0 && (s == 0 || s == 1)) {
          continue;
        }

        // Se tiene que convertir CHS de recorrido vertical a lba
        int lba = ((c * SURFACES) + h) * SECTORS_PER_TRACK + (s);

        // Si al menos un sector es usado, el bloque esta siendo usado
        if (free_space_bitmap.at(lba) == true)
          used = true;

        curr_block.push_back(lba);

        if (curr_block.size() == SECTORS_PER_BLOCK) {
          free_block_map.blocks.emplace_back(std::make_pair(used, curr_block));
          curr_block.clear();
          used = false;
        }
      }
    }
  }
}

std::vector<unsigned char> DiskManager::merge_sectors_to_block(
    std::vector<std::vector<unsigned char>> &sectors_bytes) {
  std::vector<unsigned char> merged_block;
  merged_block.reserve(SECTORS_PER_BLOCK * SECTOR_SIZE);

  for (auto &buf : sectors_bytes) {
    if (buf.size() != SECTOR_SIZE)
      throw std::runtime_error("Merge invalido");
    merged_block.insert(merged_block.end(),
                        std::make_move_iterator(buf.begin()),
                        std::make_move_iterator(buf.end()));
  }
  return merged_block;
}

std::vector<std::vector<unsigned char>> DiskManager::split_block_to_sectors(
    std::vector<unsigned char> &block_bytes) {
  std::vector<std::vector<unsigned char>> sectors_bytes;
  sectors_bytes.reserve(SECTORS_PER_BLOCK);

  auto block_it = block_bytes.begin();

  for (size_t i = 0; i < SECTORS_PER_BLOCK; ++i) {
    sectors_bytes.emplace_back(block_it, block_it + SECTOR_SIZE);
    block_it += SECTOR_SIZE;
  }

  return sectors_bytes;
}
