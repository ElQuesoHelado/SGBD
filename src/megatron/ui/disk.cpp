#include "megatron.hpp"
#include "utils.hpp"

void Megatron::ui_load_disk() {
  clearScreen();

  try {
    std::cout << "=== Cargar Disco ===\n";
    std::cout << "Nombre de disco a cargar: ";

    std::string disk_name;
    std::getline(std::cin, disk_name);

    std::cout << "Ingrese número de frames en buffer pool: ";
    size_t frames;
    if (!(std::cin >> frames) || frames == 0)
      throw std::invalid_argument("Numero de frames inválido");

    std::cout << "LRU o Clock?(0,1):  ";
    size_t is_clock;
    if (!(std::cin >> is_clock))
      throw std::invalid_argument("Tipo de buffer invalido");

    load_disk(disk_name, frames, is_clock);

    std::cout << "Disco cargado correctamente" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // FIXME: Set nulls con managers
  }

  pauseAndReturn();
}

void Megatron::ui_new_disk() {
  clearScreen();
  try {
    std::cout << "=== Crear y Cargar Disco ===\n";

    std::cout << "Nombre de disco a crear: ";

    std::string disk_name;
    std::getline(std::cin, disk_name);

    std::cout << "Ingrese número de superficies: ";
    size_t surfaces;
    if (!(std::cin >> surfaces))
      throw std::invalid_argument("Número de superficies inválido");

    std::cout << "Ingrese número de pistas por superficie: ";
    size_t tracks;
    if (!(std::cin >> tracks))
      throw std::invalid_argument("Número de pistas inválido");

    std::cout << "Ingrese número de sectores por pista: ";
    size_t sectors;
    if (!(std::cin >> sectors))
      throw std::invalid_argument("Número de sectores inválido");

    std::cout << "Ingrese número de bytes por sector: ";
    size_t bytes;
    if (!(std::cin >> bytes))
      throw std::invalid_argument("Número de bytes inválido");

    std::cout << "Ingrese número de sectores por bloque: ";
    size_t sectors_block;
    if (!(std::cin >> sectors_block))
      throw std::invalid_argument("Número de sectores por bloque invalido");

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Ingrese número de frames en buffer pool: ";
    size_t frames;
    if (!(std::cin >> frames) || frames == 0)
      throw std::invalid_argument("Numero de frames inválido");

    std::cout << "LRU o Clock?(0,1):  ";
    size_t is_clock;
    if (!(std::cin >> is_clock))
      throw std::invalid_argument("Tipo de buffer invalido");

    new_disk(disk_name, surfaces, tracks, sectors, bytes, sectors_block, frames, is_clock);

    std::cout << "\nDisco creado exitosamente\n";

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  pauseAndReturn();
}
