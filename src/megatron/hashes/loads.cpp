#include "hash/directory.hpp"
#include "megatron.hpp"

DirectoryPage Megatron::load_directory_page(size_t page_id) {
  auto &frame = buffer_manager->load_pin_page(page_id);
  std::vector<unsigned char> &page_bytes = frame.page_bytes;
  auto directory_page =
      deserialize_directory_page(page_bytes);

  buffer_manager->free_unpin_page(page_id, false);
  return directory_page;
}
