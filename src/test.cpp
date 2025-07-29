#include "megatron.hpp"
#include <string>

#include <vector>
int main(int argc, char *argv[]) {
  Megatron megatron;
  megatron.new_disk("disco2", 10, 10, 10, 500, 3, 5, 1);
  // megatron.load_disk("disco2", 5, 1);

  // megatron.load_disk("disco", 5, 1);
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

  std::string titanic = "titanic", titanic_var = "titanic_var", housing = "housing",
              name4 = "pasajero_var", empty = "", name5 = " titanic", col3 = "col3", cond = "100",
              catalog = "catalog";

  megatron.create_table(titanic, columns);

  megatron.load_CSV("csv/titanic.csv", titanic);

  serial::TableMetadata table_metadata;
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      comparisons1 = {
          {"PassengerId", ">", "300"},
          {"", "AND", ""},
          {"PassengerId", "<", "500"},
          {"", "OR", ""},
          {"Embarked", "==", "S"}},
      comparisons2 = {{"Sex", "==", "male"}}, ranged = {
                                                  {"PassengerId", "<", "15"},
                                                  {"PassengerId", ">", "13"},
                                              };

  megatron.search_table(titanic, table_metadata);

  std::string hashed_col1 = "PassengerId", hashed_col2 = "Sex";

  std::string indexed_col1 = "PassengerId", indexed_col2 = "Sex";

  megatron.add_hash_to_table(titanic, hashed_col2);
  megatron.add_index_to_table(titanic, indexed_col1);

  Comparator empty_comp{};
  // megatron.select_print(titanic, empty_comp);

  Comparator sex_comp = megatron.generate_comparator(table_metadata, comparisons2);
  Comparator ranged_comp = megatron.generate_comparator(table_metadata, ranged);

  megatron.select_print(titanic, ranged_comp);

  megatron.select_print(titanic, sex_comp);
  megatron.translate();

  // Comparator comp = megatron.generate_comparator(table_metadata, comparisons);

  // std::println("{}", megatron.get_root_page_id(table_metadata, 0));
  //  megatron.show_table_metadata(catalog);

  //  megatron.select(name3, empty, empty);
  //  megatron.select(name2, empty, empty);
  //  megatron.select(name3, col, value);
  //  megatron.select(chica, empty, empty);
}
