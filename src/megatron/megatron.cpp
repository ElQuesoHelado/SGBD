#include "megatron.hpp"
#include "disk_manager.hpp"
#include "ui.cpp"
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

void Megatron::run() {
  string opcion;
  while (!global_shutdown.load()) {
    clearScreen();
    mostrarMenu();
    getline(cin, opcion);

    if (opcion == "1")
      ui_load_disk();
    else if (opcion == "2")
      ui_new_disk();
    else if (opcion == "3")
      ui_new_table();
    else if (opcion == "4")
      ui_select_table();
    else if (opcion == "5")
      ui_select_table_condition();
    else if (opcion == "6")
      ui_find_reg();
    else if (opcion == "7")
      ui_insert_data();
    else if (opcion == "8")
      ui_update_reg();
    else if (opcion == "9")
      ui_delete_data();
    else if (opcion == "10")
      ui_delete_nth();
    else if (opcion == "11")
      ui_load_csv();
    else if (opcion == "12")
      ui_load_n_regs_csv();
    else if (opcion == "13") {
      clearScreen();
      cout << "Especificaciones de disco" << endl;
      cout << "Superficies: " << disk_manager->SURFACES << endl;
      cout << "Pistas/superf: " << disk_manager->TRACKS_PER_SURFACE << endl;
      cout << "Sectores/pista: " << disk_manager->SECTORS_PER_TRACK << endl;
      cout << "Sectores totales: " << disk_manager->TOTAL_SECTORS << endl;
      cout << "Pistas totales: " << disk_manager->TRACKS_PER_SURFACE * disk_manager->SURFACES << endl;
      cout << "Capacidad de sector: " << disk_manager->SECTOR_SIZE << endl;
      cout << "Capacidad de bloque: " << disk_manager->BLOCK_SIZE << endl;
      cout << "Bloques por pista: " << disk_manager->TRACK_SIZE / (disk_manager->BLOCK_SIZE) << endl;
      cout << "Bloques por superficie: " << disk_manager->SURFACE_SIZE / (disk_manager->BLOCK_SIZE) << endl;
      cout << "Bloques totales: " << disk_manager->DISK_CAPACITY / (disk_manager->BLOCK_SIZE) << endl;
      cout << "Bytes Usados: " << disk_manager->calculate_free_space() << " / " << disk_manager->DISK_CAPACITY << endl;

      pauseAndReturn();

      // ui_load_n_regs_csv();
    } else if (opcion == "14") {
      ui_show_table_metadata();

    } else if (opcion == "15") {
      translate();
      pauseAndReturn();

    } else if (opcion == "16") {
      set_buffer_manager_frames();
      pauseAndReturn();

    } else if (opcion == "17") {
      ui_interact_buffer_manager();
      pauseAndReturn();

    } else if (opcion == "18") {
      std::cout << "Hits/totales : "
                << buffer_manager->get_hits() << " / " << buffer_manager->get_total_accesses() << std::endl;
      pauseAndReturn();

    } else if (opcion == "20") {
      clearScreen();
      cout << "\"Cerrando el programa\"\n";
      break;
    } else {
      cout << "\nOpción inválida. Intente de nuevo.\n";
      pauseAndReturn();
    }
  }

  clean_managers();
}

// Set/Reset de frames en buffer pool
// @note Implica un flush de todas las paginas, caso ya exista un buffer_manager asignado
// FIXME: ?Sin pedir a usuario?
void Megatron::set_buffer_manager_frames() {
  std::cout << "Ingrese número de frames en buffer pool: ";
  size_t frames;
  if (!(std::cin >> frames) || frames == 0)
    throw std::invalid_argument("Numero de frames inválido");

  if (buffer_manager)
    buffer_manager->flush_all();

  buffer_manager = std::make_unique<BufferManager>(frames, disk_manager);
}

// FIXME: IMPLEMENTAR
void Megatron::load_disk(std::string disk_name, size_t n_frames) {
  if (!disk_manager)
    disk_manager = std::make_unique<DiskManager>();

  disk_manager->load_disk(disk_name);

  n_sectors_in_block = disk_manager->SECTORS_PER_BLOCK;

  set_buffer_manager_frames();
}

// FIXME: IMPLEMENTAR
void Megatron::new_disk(std::string disk_name, size_t surfaces, size_t tracks, size_t sectors, size_t bytes, size_t sectors_block) {
  if (surfaces == 0 || tracks == 0 || sectors == 0 || bytes == 0 || sectors_block == 0) {
    throw std::invalid_argument("Parámetros del disco no pueden ser cero");
  }

  if (buffer_manager)
    buffer_manager->flush_all();

  if (!disk_manager)

    n_sectors_in_block = sectors_block;

  disk_manager->new_disk(disk_name,
                         surfaces,
                         tracks,
                         sectors,
                         bytes,
                         n_sectors_in_block);

  // load_disk(disk_name);
}

void Megatron::clean_managers() {
  if (buffer_manager) {
    buffer_manager->flush_all();
    buffer_manager.reset();
  }
}

Megatron::Megatron() {
  std::signal(SIGINT, [](int) {
    global_shutdown.store(true);
  });
}

Megatron::~Megatron() {
}
