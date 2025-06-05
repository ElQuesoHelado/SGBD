#pragma once

#include <sstream>
#include <string>
#include <vector>

struct DisectedQuery {
  std::vector<std::string> fields{}, relations{}, conditions{};

  /*
   * Por ahora las querys son de tipo
   * select campo1 campo2 from rel1 rel2 where val=a val2=b
   * TODO: Checker de expresion, regex?
   */
  void process_query(std::string &query) {
    std::istringstream query_stream(query);
    std::string token;

    std::vector<std::string> *ptr_vec{nullptr};

    while (query_stream >> token) {
      if (token == "select")
        ptr_vec = &fields;
      else if (token == "from")
        ptr_vec = &relations;
      else if (token == "where")
        ptr_vec = &conditions;
      else if (ptr_vec)
        ptr_vec->push_back(token);
    }
  }

  DisectedQuery(std::string query) { process_query(query); }
};
