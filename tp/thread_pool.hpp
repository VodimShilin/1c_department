#pragma once

#include "task.hpp"
#include <thread>
#include "blocking_queue.hpp"
#include "wait_count.hpp"

#include <future>
#include <vector>

namespace exe::tp {

// Fixed-size pool of worker threads

class ThreadPool {
 public:
  explicit ThreadPool(size_t workers);
  ~ThreadPool();

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Schedules task for execution in one of the worker threads
  void Submit(Task task);

  // Waits until outstanding work count has reached zero
  void WaitIdle();

  // Stops the worker threads as soon as possible
  // Pending tasks will be discarded
  void Stop();

  // Locates current thread pool from worker thread
  static ThreadPool* Current();
  //  static ThreadPool* Current();

 private:
  void WorkerRoutine();

  // Worker threads, task queue, etc
  std::vector<std::thread> workers_;
  UnboundedBlockingQueue<Task> tasks_;
  std::atomic<bool> is_stopped_{false};
  WaitCount wait_tasks_;
};

inline ThreadPool* Current() {
  return ThreadPool::Current();
}
}  // namespace exe::tp
