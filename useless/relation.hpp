#pragma once

// #include <concepts>
// #include <cstddef>
// #include <string>
// #include <tuple>
// #include <type_traits>
// #include <unordered_map>
// #include <variant>
// #include <vector>
//
// // TODO: cambio a clase, agrupar to_string, length
// typedef std::variant<long int, double, std::string> SQL_type;
//
// template <typename T>
// concept StringCastable =
//     requires(T val) {
//       { std::to_string(val) } -> std::convertible_to<std::string>;
//     };
//
// // TODO: cambio a StringSimilar o algo asi, caso char*
// template <typename T>
// concept StringSame = std::is_same_v<std::remove_cvref_t<T>, std::string>;
//
// template <typename T>
// concept NumericType = std::integral<T> || std::floating_point<T>;
//
// template <typename... Ts>
// bool constexpr is_variant_numeric(const std::variant<Ts...> &var) {
//   return std::visit([](const auto &arg) {
//     return NumericType<std::decay_t<decltype(arg)>>;
//   },
//                     var);
// }
//
// template <typename... Ts>
// bool constexpr is_variant_textual(const std::variant<Ts...> &var) {
//   return std::visit([](const auto &arg) {
//     return StringSame<std::decay_t<decltype(arg)>>;
//   },
//                     var);
// }
//
// template <typename T>
// concept NumericSQL_type = requires(T var) {
//   { is_variant_numeric(var) } -> std::convertible_to<bool>;
//   requires is_variant_textual(var);
// };
//
// template <typename T>
// concept StringSQL_type = requires(T var) {
//   { is_variant_textual(var) } -> std::convertible_to<bool>;
//   requires is_variant_textual(var);
// };
//
// // Patron overload para visit mas limpio
// template <class... Ts>
// struct overload : Ts... {
//   using Ts::operator()...;
// };
//
// inline std::string SQL_type_to_string(SQL_type &field) {
//   return std::visit(overload{[](StringCastable auto &arg) { return std::to_string(arg); },
//                              [](StringSame auto &arg) { return arg; }},
//                     field);
// }
//
// // size_t get_length_field(StringCastable auto &field) {
// //   return std::to_string(field).length();
// // }
// //
// // size_t get_length_field(StringSame auto &field) {
// //   return field.length();
// // }
//
// inline size_t SQL_type_length(SQL_type &field) {
//   return std::visit(overload{[](StringCastable auto &arg) { return std::to_string(arg).length(); },
//                              [](StringSame auto &arg) { return arg.length(); }},
//                     field);
// }
/*
 * Contiene los datos organizados por columnas
 * -> Acceso mas conveniente, probablemente facilite la implementaciond de indexes
 * */
// struct Relation {
//   std::string name{};
//   std::vector<std::string> field_names{};
//   std::vector<std::string> types{}; // TODO: ?? Solo son usados para creacion??
//   std::vector<std::vector<SQL_type>> columns{};
//
//   size_t disk_usage();
//   size_t get_n_cols() {
//     return columns.size();
//   }
//
//   size_t get_n_rows() {
//     if (columns.empty())
//       return 0;
//     return columns.begin()->size();
//   }
// };
