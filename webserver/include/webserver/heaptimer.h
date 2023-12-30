#pragma once
#include <arpa/inet.h>
#include <assert.h>
#include <chrono>
#include <functional>
#include <time.h>
#include <unordered_map>
namespace webserver {
using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = std::chrono::high_resolution_clock::time_point;
struct TimerNode {
  int id;
  TimeStamp expires;
  TimeoutCallBack cb;
  bool operator<(const TimerNode &t) { return expires < t.expires; }
};

class HeapTimer {
private:
  void del(size_t i);
  void siftup(size_t i);
  bool siftdown(size_t index, size_t n);
  void swapnode(size_t i, size_t j);

  std::vector<TimerNode> m_heap;
  std::unordered_map<int, size_t> m_ref;

public:
  HeapTimer() { m_heap.reserve(64); }
  ~HeapTimer() { clear(); }
  void adjust(int id, int timeOut);
  void add(int id, int timeOut, const TimeoutCallBack &cb);
  void dowork(int id);
  void clear();
  void tick();
  void pop();
  int getnexttick();
};
} // namespace webserver