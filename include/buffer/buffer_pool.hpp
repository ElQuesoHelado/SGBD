#pragma once

#include "frame.hpp"
#include <list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>

class BufferPool {
public:
  BufferPool(size_t capacity) : capacity_(capacity) {}

  bool contains(size_t page_id) const {
    return frame_map_.find(page_id) != frame_map_.end();
  }

  Frame &get_frame(size_t page_id) {
    auto &entry = frame_map_.at(page_id);
    lru_list_.splice(lru_list_.begin(), lru_list_, entry.second);
    return *entry.first;
  }

  void add_frame(size_t page_id, std::unique_ptr<Frame> frame) {
    if (frame_map_.size() >= capacity_) {
      if (!evict_one_frame()) {
        throw std::runtime_error("Failed to evict frame from full buffer pool");
      }
    }
    lru_list_.push_front(page_id);
    frame_map_[page_id] = {std::move(frame), lru_list_.begin()};
  }

  bool evict_one_frame() {
    if (lru_list_.empty())
      return false;

    size_t lru_id = lru_list_.back();
    auto node = frame_map_.extract(lru_id);
    lru_list_.pop_back();

    // El caller es responsable de manejar frames sucios
    return true;
  }

  std::unique_ptr<Frame> get_frame_to_evict() {
    if (lru_list_.empty())
      return nullptr;

    size_t lru_id = lru_list_.back();
    auto node = frame_map_.extract(lru_id);
    lru_list_.pop_back();

    return std::move(node.mapped().first);
  }

  size_t size() const { return frame_map_.size(); }
  size_t capacity() const { return capacity_; }

  size_t capacity_;
  std::list<size_t> lru_list_;
  std::unordered_map<size_t, std::pair<std::unique_ptr<Frame>, std::list<size_t>::iterator>> frame_map_;

private:
};
