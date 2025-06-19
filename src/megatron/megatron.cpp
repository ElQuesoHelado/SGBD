#include "megatron.hpp"
#include "disk_manager.hpp"
#include "ui.cpp"
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
  while (true) {
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
      cout << "Superficies: " << disk.SURFACES << endl;
      cout << "Pistas/superf: " << disk.TRACKS_PER_SURFACE << endl;
      cout << "Sectores/pista: " << disk.SECTORS_PER_TRACK << endl;
      cout << "Sectores totales: " << disk.TOTAL_SECTORS << endl;
      cout << "Pistas totales: " << disk.TRACKS_PER_SURFACE * disk.SURFACES << endl;
      cout << "Capacidad de sector: " << disk.SECTOR_SIZE << endl;
      cout << "Capacidad de bloque: " << disk.BLOCK_SIZE << endl;
      cout << "Bloques por pista: " << disk.TRACK_SIZE / (disk.BLOCK_SIZE) << endl;
      cout << "Bloques por superficie: " << disk.SURFACE_SIZE / (disk.BLOCK_SIZE) << endl;
      cout << "Bloques totales: " << disk.DISK_CAPACITY / (disk.BLOCK_SIZE) << endl;
      cout << "Bytes Usados: " << disk.calculate_free_space() << " / " << disk.DISK_CAPACITY << endl;

      pauseAndReturn();

      // ui_load_n_regs_csv();
    } else if (opcion == "14") {
      ui_show_table_metadata();

    } else if (opcion == "15") {
      translate();
      pauseAndReturn();

    } else if (opcion == "16") {
      set_buffer_manager_frames();

    } else if (opcion == "20") {
      clearScreen();
      cout << "\"Cerrando el programa\"\n";
      break;
    } else {
      cout << "\nOpción inválida. Intente de nuevo.\n";
      pauseAndReturn();
    }
  }
}

// Set/Reset de frames en buffer pool
// @note Implica un flush de todas las paginas, caso ya exista un buffer_manager asignado
// FIXME: potencialmente peligroso en caso de SIGINT, check en manejo de seniales
// Cambiar a unique_ptr a disk_manager, validar caso disk no este cargado
void Megatron::set_buffer_manager_frames() {
  std::cout << "Ingrese número de frames en buffer pool: ";
  size_t frames;
  if (!(std::cin >> frames) || frames == 0)
    throw std::invalid_argument("Numero de frames inválido");

  if (buffer_manager_ptr)
    buffer_manager_ptr->flush_all();

  buffer_manager_ptr = std::make_unique<BufferManager>(frames, disk);
}

void Megatron::load_disk(std::string disk_name) {
  disk.load_disk(disk_name);

  n_sectors_in_block = disk.SECTORS_PER_BLOCK;

  set_buffer_manager_frames();
}

void Megatron::new_disk(std::string disk_name, size_t surfaces, size_t tracks, size_t sectors, size_t bytes, size_t sectors_block) {
  n_sectors_in_block = sectors_block;

  disk.new_disk(disk_name,
                surfaces,
                tracks,
                sectors,
                bytes,
                n_sectors_in_block);

  load_disk(disk_name);
}

Megatron::Megatron() {
  // if (buffer_manager_ptr)
  //   buffer_manager_ptr->flush_all();
}
