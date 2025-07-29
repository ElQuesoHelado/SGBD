#include "megatron.hpp"

int main(int argc, char *argv[]) {
  Megatron megatron;
  megatron.new_disk("disco2", 10, 10,
                    10, 500, 3,
                    5, 1);
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

  std::string titanic = "titanic", titanic_var = "titanic_var", housing = "housing",
              name4 = "pasajero_var", empty = "", name5 = " titanic", col3 = "col3", cond = "100",
              catalog = "catalog";

  megatron.create_table(titanic, columns);

  megatron.load_CSV("csv/titanic.csv",
                    "titanic");

  serial::TableMetadata table_metadata;
  std::vector<std::tuple<std::string,
                         std::string,
                         std::string>>
      comparisons1 = {{"Embarked", "==", "S"}},
      comparisons2 = {{"Sex", "==", "male"}},
      comparisons3 = {{"PassengerId", "==", "800"}},
      comparisons4 = {
          {"PassengerId", ">", "100"},
          {"PassengerId", "<", "200"},

      };
  megatron.search_table(titanic, table_metadata);
  std::string hashed_col1 = "PassengerId", hashed_col2 = "Sex";

  megatron.add_hash_to_table(titanic, hashed_col1);

  megatron.translate();

  // megatron.add_index_to_table(titanic, hashed_col1);
  //
  // Comparator equals_comp1 =
  //     megatron.generate_comparator(table_metadata, comparisons3);
  //
  // megatron.select_print(titanic, equals_comp1);
  //
  // Comparator ranged =
  //     megatron.generate_comparator(table_metadata, comparisons4);
  //
  // megatron.select_print(titanic, ranged);
  //
  // megatron.delete_condition(table_metadata, ranged);
  //
  // megatron.select_print(titanic, ranged);

  return 0;
}
