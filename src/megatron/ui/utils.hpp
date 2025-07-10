#include <iostream>

inline void clearScreen() {
  std::cout << "\033[2J\033[H";
}

inline void pauseAndReturn() {
  std::cout << "\nENTER para regresar ..." << std::flush;
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  // cin.sync();
  std::cin.get();
}

inline void showInputError(const std::string &expected = "una entrada válida") {
  std::cerr << "\nError: Se esperaba " << expected << ". Intente nuevamente.\n";
  pauseAndReturn();
}

inline void showSQLTypeTable() {
  std::cout << "\nTipos disponibles:\n";
  std::cout << "+------------------+\n";
  std::cout << "| Tipo SQL         |\n";
  std::cout << "+------------------+\n";
  std::cout << "| TINYINT          |\n";
  std::cout << "| SMALLINT         |\n";
  std::cout << "| INTEGER          |\n";
  std::cout << "| BIGINT           |\n";
  std::cout << "| FLOAT            |\n";
  std::cout << "| DOUBLE           |\n";
  std::cout << "| CHAR(N)          |\n";
  std::cout << "| VARCHAR(N)       |\n";
  std::cout << "+------------------+\n";
}

inline void mostrarMenu() {
  std::cout << "=== Gestor de Base de Datos ===\n";
  std::cout << "1. Cargar disco\n";
  std::cout << "2. Crear disco\n";
  std::cout << "3. Crear tabla\n";
  std::cout << "4. Select *\n";
  std::cout << "5. Select con condición\n";
  std::cout << "6. Ubicar registro\n";
  std::cout << "7. Insertar registro individual\n";
  std::cout << "8. Modificar por condicion\n";
  std::cout << "9. Modificar n-esimo registro\n";
  std::cout << "10. Eliminar condicion\n";
  std::cout << "11. Eliminar n-esimo registro\n";
  std::cout << "12. Cargar CSV\n";
  std::cout << "13. Cargar n datos desde CSV\n";
  std::cout << "14. Mostrar specs de disco\n";
  std::cout << "15. Mostrar metadata de tabla\n";
  std::cout << "16. Translate disco\n";
  std::cout << "17. Set #frames por buffer pool\n";
  std::cout << "18. Interactuar Buffer Manager\n";
  std::cout << "19. Mostrar hits\n";
  std::cout << "20. Agregar columna hash a tabla\n";
  std::cout << "0. Salir\n";
  std::cout << "Seleccione una opción: ";
}
