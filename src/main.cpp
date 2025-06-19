#include "megatron.hpp"
#include <utility>

int main(int argc, char *argv[]) {
  Megatron megatron;
  // //
  megatron.new_disk("disco2", 10, 10, 10, 500, 3);
  // //
  // megatron.load_disk("disco");
  // megatron.load_CSV("csv/titanic.csv", "titanic", 50);
  // std::string str = "titanic";
  // std::string cond = "", val = "";
  //
  // megatron.select(str, cond, val);
  // //
  // // //
  std::vector<std::pair<std::string, std::string>>
      columns = {
          {"PassengerId", "INTEGER"},
          {"Survived", "INTEGER"},
          {"Pclass", "INTEGER"},
          {"Name", "CHAR(100)"},
          {"Sex", "CHAR(20)"},
          {"Age", "DOUBLE"},
          {"SibSp", "INTEGER"},
          {"Parch", "INTEGER"},
          {"Ticket", "CHAR(20)"},
          {"Fare", "DOUBLE"},
          {"Cabin", "CHAR(20)"},
          {"Embarked", "CHAR(20)"}};
  // //
  // // std::vector<std::pair<std::string, std::string>>
  // //     columns_var = {
  // //         {"PassengerId", "INTEGER"},
  // //         {"Survived", "INTEGER"},
  // //         {"Pclass", "INTEGER"},
  // //         {"Name", "VARCHAR(100)"},
  // //         {"Sex", "VARCHAR(20)"},
  // //         {"Age", "DOUBLE"},
  // //         {"SibSp", "INTEGER"},
  // //         {"Parch", "INTEGER"},
  // //         {"Ticket", "VARCHAR(20)"},
  // //         {"Fare", "DOUBLE"},
  // //         {"Cabin", "VARCHAR(20)"},
  // //         {"Embarked", "VARCHAR(20)"}};
  // //
  megatron.create_table("titanic", columns);
  // // megatron.create_table("titanic_var", columns_var);
  // // //
  // // //
  // std::vector<std::pair<std::string, std::string>> columns = {{"col1", "INTEGER"},
  //                                                             {"col2", "INTEGER"},
  //                                                             {"col3", "INTEGER"},
  //                                                             {"col4", "CHAR(100)"},
  //                                                             {"col5", "CHAR(20)"}};
  // std::vector<std::pair<std::string, std::string>> columns_var = {{"col1", "INTEGER"},
  //                                                                 {"col2", "INTEGER"},
  //                                                                 {"col3", "INTEGER"},
  //                                                                 {"col4", "VARCHAR(100)"},
  //                                                                 {"col5", "VARCHAR(20)"}};
  // megatron.create_table("chica", columns);
  // // //
  // // megatron.create_table("chica_var", columns_var);
  // // //
  // // // // serial::TableMetadata table_metadata;
  // // // // std::cout << megatron.search_table("tabla2", table_metadata) << std::endl;
  // // // // megatron.load_disk("disco_ejemplo");
  // // // megatron.load_CSV("csv/chica.csv", "chica_var");
  // // // megatron.load_CSV("csv/titanic_chica.csv", "titanic");
  // megatron.load_CSV("csv/titanic.csv", "titanic");
  // // megatron.load_CSV("csv/titanic.csv", "titanic_var");
  // //
  // // // // megatron.load_CSV("csv/mas_chica.csv", "chica_var");
  // // //
  // // // // megatron.run();
  // // //
  // std::string name1 = "titanic", name2 = "titanic_var", name3 = "housing",
  //             name4 = "pasajero_var", empty = "", chica = "chica", col3 = "col3", cond = "100";
  //
  // megatron.load_CSV("csv/chica.csv", chica);
  //
  // megatron.delete_reg(chica, col3, cond);
  //
  // megatron.load_CSV("csv/chica.csv", chica);

  // // //
  // std::string col = "Pclass", value = "3";
  //
  // megatron.delete_reg(name1, col, value);
  //
  megatron.load_CSV("csv/titanic.csv", "titanic", 24);
  // // megatron.delete_reg(name2, col, value);
  //
  // megatron.select(name3, empty, empty);
  // megatron.select(name2, empty, empty);
  // megatron.select(name3, col, value);
  // megatron.select(chica, empty, empty);
  // megatron.run();
  return 0;
}
