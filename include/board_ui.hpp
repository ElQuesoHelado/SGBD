#include "board.hpp"
#include <iostream>
#include <limits>
#include <optional>
#include <string>

class BoardInterface {
public:
  void run() {
    board.translate();

    while (true) {
      clear_screen();
      print_menu();

      int choice = get_numeric_input("Seleccione una opción: ");

      switch (choice) {
      case 1:
        handle_get_all_boards();
        break;
      case 2:
        handle_get_all_threads();
        break;
      case 3:
        handle_get_all_posts();
        break;
      case 4:
        handle_get_board_on_id();
        break;
      case 5:
        handle_get_thread_on_id();
        break;
      case 6:
        handle_get_post_on_id();
        break;
      case 7:
        handle_get_threads_from_board();
        break;
      case 8:
        handle_get_posts_from_board();
        break;
      case 9:
        handle_get_posts_from_thread();
        break;
      case 10:
        handle_get_all_threads_ordered_by_date();
        break;
      case 11:
        handle_get_all_posts_ordered_by_date();
        break;
      case 12:
        handle_get_threads_from_board_ordered_by_date();
        break;
      case 13:
        handle_get_posts_from_board_ordered_by_date();
        break;
      case 14:
        handle_get_posts_from_thread_ordered_by_date();
        break;
      case 15:
        handle_get_all_posts_in_date_range();
        break;
      case 16:
        handle_get_posts_in_date_range_from_thread();
        break;
      case 17:
        handle_get_posts_in_date_range_from_board();
        break;
      case 18:
        handle_delete_posts_in_date_range();
        break;
      case 19:
        handle_delete_thread();
        break;
      case 20:
        handle_insert();
        break;
      case 21:
        handle_show_page();
        break;
      case 0:
        return; // Salir
      default:
        std::cout << "Opción no válida. Intente de nuevo.\n";
        wait_for_enter();
      }
    }
  }

  BoardInterface(Board &board) : board(board) {}

private:
  void print_menu() {
    std::cout << "=== MENÚ PRINCIPAL ===\n"
              << "1. Obtener todos los tableros\n"
              << "2. Obtener todos los hilos\n"
              << "3. Obtener todos los posts\n"
              << "4. Obtener tablero por ID\n"
              << "5. Obtener hilo por ID\n"
              << "6. Obtener post por ID\n"
              << "7. Obtener hilos de un tablero\n"
              << "8. Obtener posts de un tablero\n"
              << "9. Obtener posts de un hilo\n"
              << "10. Obtener todos los hilos ordenados por fecha\n"
              << "11. Obtener todos los posts ordenados por fecha\n"
              << "12. Obtener hilos de tablero ordenados por fecha\n"
              << "13. Obtener posts de tablero ordenados por fecha\n"
              << "14. Obtener posts de hilo ordenados por fecha\n"
              << "15. Obtener posts en rango de fechas\n"
              << "16. Obtener posts en rango de fechas de hilo\n"
              << "17. Obtener posts en rango de fechas de tablero\n"
              << "18. Eliminar posts en rango de fechas\n"
              << "19. Eliminar hilo\n"
              << "20. Insertar posts\n"
              << "21. Mostrar pagina\n"
              << "0. Salir\n\n";
  }

  void handle_insert() {
    size_t n = get_numeric_input<size_t>("Ingrese n_posts a insertar: ");
    board.load_posts(n);
    board.translate();
    wait_for_enter();
  }

  void handle_show_page() {
    size_t nth_page = get_numeric_input<size_t>("Ingrese numero pagina: ");
    std::string path = "bloques/bloque " + std::to_string(nth_page) + ".txt";
    std::ifstream block(path);

    if (block) {
      std::string line;
      while (std::getline(block, line)) {
        std::cout << line << std::endl;
      }
    }
    wait_for_enter();
  }

  // Funciones de manejo para cada opción
  void handle_get_all_boards() {
    std::cout << "=== Todos los tableros ===\n";
    std::cout << board.get_all_boards() << "\n";
    wait_for_enter();
  }

  void handle_get_all_threads() {
    std::cout << "=== Todos los hilos ===\n";
    std::cout << board.get_all_threads() << "\n";
    wait_for_enter();
  }

  void handle_get_all_posts() {
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Todos los posts ===\n";
    auto result = board.get_all_posts();
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_get_board_on_id() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del tablero: ");
    std::cout << "=== Tablero con ID " << id << " ===\n";
    std::cout << board.get_board_on_id(id) << "\n";
    wait_for_enter();
  }

  void handle_get_thread_on_id() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del hilo: ");
    std::cout << "=== Hilo con ID " << id << " ===\n";
    std::cout << board.get_thread_on_id(id) << "\n";
    wait_for_enter();
  }

  void handle_get_post_on_id() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del post: ");
    std::cout << "=== Post con ID " << id << " ===\n";
    std::cout << board.get_post_on_id(id) << "\n";
    wait_for_enter();
  }

  void handle_get_threads_from_board() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del tablero: ");
    std::cout << "=== Hilos del tablero " << id << " ===\n";
    std::cout << board.get_threads_from_board(id) << "\n";
    wait_for_enter();
  }

  void handle_get_posts_from_board() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del tablero: ");
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Posts del tablero " << id << " ===\n";
    auto result = board.get_posts_from_board(id);
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_get_posts_from_thread() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del hilo: ");
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Posts del hilo " << id << " ===\n";
    auto result = board.get_posts_from_thread(id);
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_get_all_threads_ordered_by_date() {
    bool asc = get_yes_no_input("¿Orden ascendente? (s/n): ");
    std::cout << "=== Todos los hilos ordenados por fecha ===\n";
    std::cout << board.get_all_threads_ordered_by_date(asc) << "\n";
    wait_for_enter();
  }

  void handle_get_all_posts_ordered_by_date() {
    bool asc = get_yes_no_input("¿Orden ascendente? (s/n): ");
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Todos los posts ordenados por fecha ===\n";
    auto result = board.get_all_posts_ordered_by_date(asc);
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_get_threads_from_board_ordered_by_date() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del tablero: ");
    bool asc = get_yes_no_input("¿Orden ascendente? (s/n): ");
    std::cout << "=== Hilos del tablero " << id << " ordenados por fecha ===\n";
    std::cout << board.get_threads_from_board_ordered_by_date(id, asc) << "\n";
    wait_for_enter();
  }

  void handle_get_posts_from_board_ordered_by_date() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del tablero: ");
    bool asc = get_yes_no_input("¿Orden ascendente? (s/n): ");
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Posts del tablero " << id << " ordenados por fecha ===\n";
    auto result = board.get_posts_from_board_ordered_by_date(id, asc);
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_get_posts_from_thread_ordered_by_date() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del hilo: ");
    bool asc = get_yes_no_input("¿Orden ascendente? (s/n): ");
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Posts del hilo " << id << " ordenados por fecha ===\n";
    auto result = board.get_posts_from_thread_ordered_by_date(id, asc);
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_get_all_posts_in_date_range() {
    size_t low = get_numeric_input<size_t>("Ingrese fecha inicial: ");
    size_t high = get_numeric_input<size_t>("Ingrese fecha final: ");
    bool asc = get_yes_no_input("¿Orden ascendente? (s/n): ");
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Posts en rango de fechas ===\n";
    auto result = board.get_all_posts_in_date_range(low, high, asc);
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_get_posts_in_date_range_from_thread() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del hilo: ");
    size_t low = get_numeric_input<size_t>("Ingrese fecha inicial: ");
    size_t high = get_numeric_input<size_t>("Ingrese fecha final: ");
    bool asc = get_yes_no_input("¿Orden ascendente? (s/n): ");
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Posts del hilo " << id << " en rango de fechas ===\n";
    auto result = board.get_posts_in_date_range_from_thread(id, low, high, asc);
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_get_posts_in_date_range_from_board() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del tablero: ");
    size_t low = get_numeric_input<size_t>("Ingrese fecha inicial: ");
    size_t high = get_numeric_input<size_t>("Ingrese fecha final: ");
    bool asc = get_yes_no_input("¿Orden ascendente? (s/n): ");
    bool silent = get_yes_no_input("¿Output silencioso? (s/n): ");
    std::cout << "=== Posts del tablero " << id << " en rango de fechas ===\n";
    auto result = board.get_posts_in_date_range_from_board(id, low, high, asc);
    if (!silent) {
      std::cout << result << "\n";
    }
    wait_for_enter();
  }

  void handle_delete_posts_in_date_range() {
    size_t low = get_numeric_input<size_t>("Ingrese fecha inicial: ");
    size_t high = get_numeric_input<size_t>("Ingrese fecha final: ");
    std::cout << "=== Eliminando posts en rango de fechas ===\n";
    std::cout << board.delete_posts_in_date_range(low, high) << "\n";
    board.translate();

    wait_for_enter();
  }

  void handle_delete_thread() {
    size_t id = get_numeric_input<size_t>("Ingrese ID del hilo a eliminar: ");
    std::cout << "=== Eliminando hilo " << id << " ===\n";
    std::cout << "Resultado de la eliminación:\n";
    std::cout << board.delete_thread(id) << "\n";

    board.translate();

    wait_for_enter();
  }

  // Funciones auxiliares
  template <typename T = int>
  T get_numeric_input(const std::string &prompt) {
    T value;
    while (true) {
      std::cout << prompt;
      std::cin >> value;

      if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Entrada no válida. Por favor ingrese un número.\n";
      } else {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return value;
      }
    }
  }

  bool get_yes_no_input(const std::string &prompt) {
    char response;
    while (true) {
      std::cout << prompt;
      std::cin >> response;
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      if (response == 's' || response == 'S')
        return true;
      if (response == 'n' || response == 'N')
        return false;

      std::cout << "Por favor ingrese 's' o 'n'.\n";
    }
  }

  void wait_for_enter() {
    std::cout << "\nPresione Enter para continuar...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  void clear_screen() {
    std::cout << "\033[2J\033[H";
  }

  // Asumo que tienes una instancia de tu clase Board disponible
  Board &board; // Reemplaza Board con el nombre real de tu clase
};
