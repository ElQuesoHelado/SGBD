#pragma once

#include "buffer_pool.hpp"
#include "disk_manager.hpp"
#include "frame.hpp"
#include <iostream>

class BufferManager {
public:
  BufferManager(size_t capacity, DiskManager &disk_manager)
      : pool_(capacity), disk_manager_(disk_manager) {}

  Frame &get_block(size_t block_id) {
    if (pool_.contains(block_id)) {
      return pool_.get_frame(block_id);
    }
    return load_block(block_id);
  }

  void mark_dirty(size_t block_id) {
    pool_.get_frame(block_id).dirty = true;
  }

  void flush_block(size_t block_id) {
    if (pool_.contains(block_id)) {
      auto &frame = pool_.get_frame(block_id);
      if (frame.dirty) {
        disk_manager_.write_block(frame.page_bytes, block_id);
        frame.dirty = false;
      }
    }
  }

  void flush_all() {
    while (auto dirty_frame = pool_.evict_lru()) {
      disk_manager_.write_block(dirty_frame->page_bytes, dirty_frame->page_id);
    }
  }

private:
  Frame &load_block(size_t block_id) {
    if (pool_.size() >= pool_.capacity()) {
      make_space();
    }

    auto frame = std::make_unique<Frame>();
    frame->page_id = block_id;
    frame->dirty = false;
    disk_manager_.read_block(frame->page_bytes, block_id);

    auto &ref = *frame;
    pool_.add_frame(block_id, std::move(frame));
    return ref;
  }

  void make_space() {
    while (auto dirty_frame = pool_.evict_lru()) {
      disk_manager_.write_block(dirty_frame->page_bytes, dirty_frame->page_id);
      // std::cout << "EVICTED DIRTY" << std::endl;
    }
  }

  BufferPool pool_;
  DiskManager &disk_manager_;
};
