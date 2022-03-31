#include <condition_variable>
#include <mutex>

namespace exe::tp {

class WaitCount {
 public:
  void Inc() {
    std::lock_guard guard(mutex_);
    ++count_;
    if (count_ == 0) {
      is_zero_.notify_all();
    }
  }

  void Dec() {
    std::lock_guard guard(mutex_);
    --count_;
    if (count_ == 0) {
      is_zero_.notify_all();
    }
  }

  void Wait() {
    std::unique_lock guard(mutex_);
    while (count_ != 0) {
      is_zero_.wait(guard);
    }
  }

 private:
  std::condition_variable is_zero_;
  std::mutex mutex_;
  int64_t count_ = 0;
};

}  // namespace exe::tp