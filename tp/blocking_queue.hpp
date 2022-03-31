#pragma once

#include <mutex>
#include <condition_variable>

#include <optional>
#include <deque>
#include <cassert>
#include <atomic>

namespace exe::tp {

// Unbounded blocking multi-producers/multi-consumers queue

template <typename T>
class UnboundedBlockingQueue {
 public:
  bool Put(T value) {
    std::lock_guard guard(mutex_);
    if (is_close_.load()) {
      return false;
    }
    buffer_.emplace_back(std::move(value));
    not_empty_.notify_one();
    return true;
  }

  std::optional<T> Take() {
    std::unique_lock guard(mutex_);
    while (buffer_.empty() && !is_close_.load()) {
      not_empty_.wait(guard);
    }
    return is_close_.load() && buffer_.empty() ? std::optional<T>(std::nullopt)
                                               : TakeLocked();
  }

  void Close() {
    CloseImpl(/*clear=*/false);
  }

  void Cancel() {
    CloseImpl(/*clear=*/true);
  }

  ~UnboundedBlockingQueue() {
    assert(buffer_.empty());
  }

 private:
  void CloseImpl(bool clear) {
    std::lock_guard guard(mutex_);
    is_close_.store(true);
    if (clear) {
      buffer_.clear();
    }
    not_empty_.notify_all();
  }

  T TakeLocked() {
    assert(!buffer_.empty());
    T front{std::move(buffer_.front())};
    buffer_.pop_front();
    return front;
  }

 private:
  std::deque<T> buffer_;  // Guarded by mutex_
  std::mutex mutex_;
  std::condition_variable not_empty_;
  std::atomic<bool> is_close_{false};
};

}  // namespace exe::tp
