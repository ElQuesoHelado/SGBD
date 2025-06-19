#include "megatron.hpp"
#include "serial/fixed_data.hpp"
#include "serial/page_header.hpp"
#include "serial/slotted_data.hpp"
#include "serial/table.hpp"
#include <cstdint>

// Implica modificar page y metadata de tabla
// Se inserta al final
// @notes Escribe tanto un page_header nuevo y un Data_header(depende de fixed o slotted)
// FIXME: Cambio a buffer_manager probablemente implique hacer un "reserve" de un bloque
uint32_t Megatron::add_new_page_to_table(serial::TableMetadata &table_metadata) {
  // Se carga ultima pagina
  auto last_page_id = table_metadata.last_page_id;

  // auto &frame = buffer_manager_ptr->get_block(last_page_id);
  // std::vector<unsigned char> &last_page_bytes = frame.page_bytes;
  std::vector<unsigned char> last_page_bytes;
  disk.read_block(last_page_bytes, table_metadata.last_page_id);

  auto page_header = serial::deserialize_page_header(last_page_bytes);

  if (page_header.next_block_id != disk.NULL_BLOCK)
    throw std::runtime_error("Ultima pagina de tabla no tiene puntero a null");

  // Se crea nueva pagina de acuerdo a formato de table_metadata
  auto new_page_id = create_page(table_metadata);

  // Se actualiza puntero a next de ultima pagina y ptr a last page de tabla
  page_header.next_block_id = new_page_id;
  table_metadata.last_page_id = new_page_id;

  // Se escriben pagina nueva
  auto overwrite_it = last_page_bytes.begin();
  serial::serialize_page_header(page_header, overwrite_it);

  // Escribe tabla con nuevo puntero a pagina nueva(la ultima)
  auto table_block_bytes =
      serial::serialize_table_metadata(table_metadata, disk.BLOCK_SIZE);
  disk.write_block(table_block_bytes, table_metadata.table_block_id);

  // frame.dirty = true;

  // Escribe la ultima pagina con puntero actualizado
  disk.write_block(last_page_bytes, last_page_id);

  return new_page_id;
}

/*
 * Se crea una pagina basada en una tabla
 * Escribe en disco,
 * @return block_id de pagina nueva
 */
uint32_t Megatron::create_page(serial::TableMetadata &table_metadata) {
  serial::FixedDataHeader fixed_data_header;
  serial::SlottedDataHeader slotted_data_header;
  serial::PageHeader page_header;

  // Se concatena page_header + data_header + fill
  std::vector<unsigned char> page_bytes(disk.BLOCK_SIZE);
  auto write_it = page_bytes.begin();

  if (table_metadata.are_regs_fixed) {
    init_fixed_data_header(table_metadata, fixed_data_header);
    init_page_header(page_header, fixed_data_header.free_bytes);

    serial::serialize_page_header(page_header, write_it);
    serial::serialize_fixed_block_header(fixed_data_header, write_it);
  } else {
    init_slotted_data_header(table_metadata, slotted_data_header);
    init_page_header(page_header, slotted_data_header.free_bytes);

    serial::serialize_page_header(page_header, write_it);
    serial::serialize_slotted_data_header(slotted_data_header, write_it);
  }

  return disk.write_block(page_bytes);
}
