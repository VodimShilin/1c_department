#include "thread_pool.hpp"

//#include <twist/util/thread_local.hpp>

namespace exe::tp {

////////////////////////////////////////////////////////////////////////////////

//static twist::util::ThreadLocalPtr<ThreadPool> pool;

////////////////////////////////////////////////////////////////////////////////

ThreadPool::ThreadPool(size_t workers) {
  for (size_t i = 0; i < workers; ++i) {
    workers_.emplace_back([this]() {
      WorkerRoutine();
    });
  }
}

ThreadPool::~ThreadPool() {
  assert(is_stopped_);
}

void ThreadPool::Submit(Task task) {
  if (tasks_.Put(std::move(task))) {
    wait_tasks_.Inc();
  }
}

///Считать кол-во пришедших-ушедших задач
///Посмотреть waitgroup

void ThreadPool::WaitIdle() {
  wait_tasks_.Wait();
}

void ThreadPool::Stop() {
  is_stopped_.store(true);
  tasks_.Cancel();

  for (auto& worker : workers_) {
    worker.join();
  }
}

//ThreadPool* ThreadPool::Current() {
//  return pool;
//}

void ThreadPool::WorkerRoutine() {
//  pool = this;
  while (!is_stopped_.load()) {
    auto task = tasks_.Take();
    if (task.has_value()) {
      try {
        task.value()();
      } catch (...) {
      }
      wait_tasks_.Dec();
    } else {
      break;
    }
  }
}

}  // namespace exe::tp
