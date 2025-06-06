#include "megatron.hpp"
#include "disk_manager.hpp"
#include "ui.cpp"
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
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
      ui_insert_data();
    else if (opcion == "7")
      ui_update_reg();
    else if (opcion == "8")
      ui_delete_data();
    else if (opcion == "9")
      ui_load_csv();
    else if (opcion == "10")
      ui_load_n_regs_csv();
    else if (opcion == "11") {
      clearScreen();
      cout << "Especificaciones de disco" << endl;
      cout << "Capacidad del disco: " << disk.DISK_CAPACITY << " bytes    " << disk.DISK_CAPACITY / 8 << " KiB" << endl;
      cout << "Capacidad de sector: " << disk.SECTOR_SIZE << endl;
      cout << "Capacidad de bloque: " << disk.BLOCK_SIZE << endl;
      cout << "Bloques por pista: " << disk.TRACK_SIZE / (disk.BLOCK_SIZE) << endl;
      cout << "Bloques por plato: " << disk.SURFACE_SIZE / (disk.BLOCK_SIZE) << endl;

      pauseAndReturn();

      // ui_load_n_regs_csv();
    } else if (opcion == "12") {
      translate();
      pauseAndReturn();

    } else if (opcion == "13") {
      clearScreen();
      cout << "\"Cerrando el programa\"\n";
      break;
    } else {
      cout << "\nOpción inválida. Intente de nuevo.\n";
      pauseAndReturn();
    }
  }
}

void Megatron::load_disk(std::string disk_name) {
  disk.load_disk(disk_name);
  // schemas_file.open("schema/schemas.txt", std::ios::in | std::ios::out | std::ios::app);
  // // std::ifstream metad_file(metad_path);
  // metad_file >> n_sectors_in_block;
  n_sectors_in_block = disk.SECTORS_PER_BLOCK;
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
}
