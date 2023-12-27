#pragma once
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace webserver {

template <typename T> class SafeQueue {
private:
  std::queue<T> m_queue;
  std::mutex m_mutex;

public:
  SafeQueue() {}
  SafeQueue(SafeQueue &&other) {}
  ~SafeQueue() {}

  bool empty() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.empty();
  }

  int size() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.size();
  }

  void enqueue(T &t) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.emplace(t);
  }

  bool dequeue(T &t) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_queue.empty())
      return false;
    t = std::move(m_queue.front());
    m_queue.pop();
    return true;
  }
};

class ThreadPool {
private:
  bool m_shutdown;
  SafeQueue<std::function<void()>> m_queue;
  std::vector<std::jthread> m_threads;
  std::mutex m_conditional_mutex;
  std::condition_variable m_conditional_cv;

  class ThreadWorker {
  private:
    int m_id;
    ThreadPool *m_pool;

  public:
    ThreadWorker(ThreadPool *pool, const int id) : m_pool(pool), m_id(id) {}

    void operator()() {
      while (!m_pool->m_shutdown) {
        bool dequeued = false;
        std::function<void()> func;
        {
          std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
          if (m_pool->m_queue.empty()) {
            m_pool->m_conditional_cv.wait(lock);
          }
          dequeued = m_pool->m_queue.dequeue(func);
        }
        if (dequeued)
          func();
      }
    }
  };

public:
  ThreadPool(const int n_threads = 4)
      : m_threads(std::vector<std::jthread>(n_threads)), m_shutdown(false) {}

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool &operator=(const ThreadPool&) = delete;
  ThreadPool &operator=(const ThreadPool&&) = delete;

  void init() {
    for(int i = 0; i < m_threads.size(); i++) {
      m_threads[i] = std::jthread(ThreadWorker(this, i));
    }
  }

  void shutdown() {
    m_shutdown = true;
    m_conditional_cv.notify_all();
  }

  template<typename F, typename... Args>
  auto submit(F &&f, Args &&... args)->std::future<decltype(f(args...))> {
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
    std::function<void()> warpper_func = [task_ptr](){
        (*task_ptr)();
    };
    m_queue.enqueue(warpper_func);
    m_conditional_cv.notify_one();
    return task_ptr->get_future(); 
  };
};

}; // namespace webserver