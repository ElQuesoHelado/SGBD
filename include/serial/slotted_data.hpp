#pragma once

#include "serial/generic.hpp"
#include "serial/page_header.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>

namespace serial {

#pragma pack(push, 1)
struct Slot {
  uint16_t reg_size{};
  uint16_t offset_reg_start{};
  uint8_t is_used{}; // Borrado o no
};
#pragma pack(pop)

struct SlottedDataHeader {
  uint16_t free_bytes{};
  uint16_t n_slots{};
  uint16_t free_space_offset{};
  std::vector<Slot> slots{};
};

inline size_t calculate_slotted_data_header_size(const SlottedDataHeader &header) {
  return sizeof(header.free_bytes) +
         sizeof(header.n_slots) +
         sizeof(header.free_space_offset) +
         header.n_slots * sizeof(serial::Slot);
}

inline size_t base_slotted_data_header_size(const SlottedDataHeader &header) {
  return sizeof(header.free_bytes) +
         sizeof(header.n_slots) +
         sizeof(header.free_space_offset);
}

/*
 * Actualiza header y slot para aceptar un registro de reg_size bytes
 * El slot a elegir tiene que estar libre y existir
 * @return offset donde empieza dicho registro
 */
inline size_t insert_into_slotted(SlottedDataHeader &header, size_t n_slot, size_t reg_size) {
  if (n_slot >= header.slots.size())
    throw std::runtime_error("Slot fuera de rango");

  auto &slot = header.slots[n_slot];
  if (reg_size > header.free_bytes || slot.is_used)
    throw std::runtime_error("SlottedDataHeader invalido: sin capacidad suficiente o slot usado");

  // Solo se resta el tamaño real del registro; el slot ya existe
  header.free_bytes -= reg_size;
  header.free_space_offset -= reg_size;

  slot.offset_reg_start = header.free_space_offset;
  slot.reg_size = reg_size;
  slot.is_used = true;

  return slot.offset_reg_start;
}

/*
 * Se busca un slot libre
 * @return numero del slot libre, este se limpia
 * caso no haya ninguno, retorna n_slots
 */
inline size_t get_free_slot(SlottedDataHeader &slotted_data_header) {
  size_t slot_pos{};
  for (auto &s : slotted_data_header.slots) {
    if (!s.is_used) {
      s = {}; // limpia
      return slot_pos;
    }

    slot_pos++;
  }

  return slotted_data_header.n_slots;
}

/*
 * Agrega un slot libre
 * Implica actualizar page_header
 * @return numero de nuevo slot libre
 */
inline size_t add_free_slot(PageHeader &page_header, SlottedDataHeader &slotted_data_header) {
  if (slotted_data_header.free_bytes < sizeof(Slot))
    throw std::runtime_error("No hay espacio para más slots");

  slotted_data_header.slots.emplace_back(); // vacio
  slotted_data_header.n_slots++;
  slotted_data_header.free_bytes -= sizeof(Slot);

  page_header.free_space -= sizeof(Slot);

  return slotted_data_header.n_slots - 1;
}

inline std::vector<unsigned char> serialize_slotted_data_header(const SlottedDataHeader &header) {
  size_t size = calculate_slotted_data_header_size(header);

  std::vector<unsigned char> bytes;
  bytes.reserve(size);

  write_v(bytes, header.free_bytes);
  write_v(bytes, header.n_slots); // Garantiza que nunca sea != de n_slots
  write_v(bytes, header.free_space_offset);

  for (auto &S : header.slots)
    write_v(bytes, S);

  return bytes;
}

template <typename Iter>
inline void serialize_slotted_data_header(const SlottedDataHeader &header, Iter &out_it) {
  write_v(out_it, header.free_bytes);
  write_v(out_it, header.n_slots); // Garantiza que nunca sea != de n_slots
  write_v(out_it, header.free_space_offset);

  for (auto &S : header.slots)
    write_v(out_it, S);
}

// Siempre esta despues de un PageHeader, se considera como offset
inline SlottedDataHeader deserialize_slotted_data_header(std::vector<unsigned char> &bytes) {
  serial::SlottedDataHeader header;
  auto it = bytes.begin() + sizeof(PageHeader);

  read_v(it, header.free_bytes);
  read_v(it, header.n_slots);
  read_v(it, header.free_space_offset);

  header.slots.resize(header.n_slots);
  for (uint16_t i{}; i < header.n_slots; ++i) {
    read_v(it, header.slots[i]);
  }

  return header;
}

template <typename Iter>
inline SlottedDataHeader deserialize_slotted_data_header(Iter &in_it) {
  serial::SlottedDataHeader header;

  read_v(in_it, header.free_bytes);
  read_v(in_it, header.n_slots);
  read_v(in_it, header.free_space_offset);

  header.slots.resize(header.n_slots);
  for (uint16_t i{}; i < header.n_slots; ++i) {
    read_v(in_it, header.slots[i]);
  }

  return header;
}
} // namespace serial
