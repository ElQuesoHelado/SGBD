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
    if (!pool_.contains(block_id)) {
      load_block(block_id);
    }
    return pool_.get_frame(block_id);
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
    while (auto frame = pool_.get_frame_to_evict()) {
      if (frame->dirty) {
        disk_manager_.write_block(frame->page_bytes, frame->page_id);
      }
    }
  }

private:
  void load_block(size_t block_id) {
    while (pool_.size() >= pool_.capacity()) {
      if (!try_evict_one_frame()) {
        throw std::runtime_error("Failed to make space in buffer pool");
      }
    }

    std::vector<unsigned char> data;
    disk_manager_.read_block(data, block_id);

    auto frame = std::make_unique<Frame>(block_id, std::move(data));
    pool_.add_frame(block_id, std::move(frame));
  }

  bool try_evict_one_frame() {
    auto frame = pool_.get_frame_to_evict();
    if (!frame)
      return false;

    if (frame->dirty) {
      disk_manager_.write_block(frame->page_bytes, frame->page_id);
    }
    return true;
  }

  BufferPool pool_;
  DiskManager &disk_manager_;
};
