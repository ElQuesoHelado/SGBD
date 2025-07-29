#include "megatron.hpp"
#include "utils.hpp"

void Megatron::ui_show_page() {
  using namespace std;
  clearScreen();

  translate();

  try {
    size_t nth_page;
    cout << "Numero de pagina: ";
    if (!(std::cin >> nth_page) || nth_page >= disk_manager->NULL_BLOCK)
      throw std::invalid_argument("Número de pagina inválido");

    string path = "bloques/bloque " + std::to_string(nth_page) + ".txt";
    std::ifstream block(path);

    if (!block)
      throw std::invalid_argument("Bloque no traducido");

    string line;
    while (std::getline(block, line)) {
      std::cout << line << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << "\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  pauseAndReturn();
}
