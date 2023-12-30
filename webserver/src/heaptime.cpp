#include <webserver/heaptimer.h>

namespace webserver {

void HeapTimer::del(size_t i) {
  assert(!m_heap.empty() && i >= 0 && i < m_heap.size());
  size_t n = m_heap.size() - 1;
  assert(i <= n);
  if(i < n) {
    swapnode(i, n);
    if(!siftdown(i, n)) siftup(i);
  }
  m_ref.erase(m_heap.back().id);
  m_heap.pop_back();
}

void HeapTimer::siftup(size_t i) {
  assert(i >= 0 && i < m_heap.size());
  size_t j = (i - 1) / 2;
  while (j >= 0) {
    if (m_heap[j] < m_heap[i])
      break;
    swapnode(i, j);
    i = j;
    j = (i - 1) / 2;
  }
}

bool HeapTimer::siftdown(size_t index, size_t n) {
  assert(index >= 0 && index < m_heap.size());
  assert(n >= 0 && n <= m_heap.size());
  size_t i = index;
  size_t j = i * 2 + 1;
  while (j < n) {
    if (j + 1 < n && m_heap[j + 1] < m_heap[j])
      j++;
    if (m_heap[i] < m_heap[j])
      break;
    swapnode(i, j);
    i = j;
    j = i * 2 + 1;
  }
  return i > index;
}

void HeapTimer::swapnode(size_t i, size_t j) {
  assert(i >= 0 && i < m_heap.size());
  assert(j >= 0 && j < m_heap.size());
  std::swap(m_heap[i], m_heap[j]);
  m_ref[m_heap[i].id] = i;
  m_ref[m_heap[j].id] = j;
}

int HeapTimer::getnexttick() {
  tick();
  size_t res = -1;
  if(!m_heap.empty()) {
    res = std::chrono::duration_cast<MS>(m_heap.front().expires - Clock::now()).count();
    if(res < 0) res = 0;
  }
  return res;
}

void HeapTimer::pop() {
  assert(!m_heap.empty());
  del(0);
}

void HeapTimer::tick() {
  if(m_heap.empty()) return;
  while(!m_heap.empty()) {
    auto node = m_heap.front();
    if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) break;
    node.cb();
    pop();
  }
}

void HeapTimer::clear() {
  m_ref.clear();
  m_heap.clear();
}

void HeapTimer::dowork(int id) {
  if(m_heap.empty() || !m_ref.count(id)) return;
  size_t i = m_ref[id];
  m_heap[i].cb();
  del(i);
}

void HeapTimer::add(int id, int timeOut, const TimeoutCallBack &cb) {
  assert(id >= 0);
  size_t i;
  if(!m_ref.count(id)) {
    i = m_heap.size();
    m_ref[id] = i;
    m_heap.push_back({id, Clock::now() + MS(timeOut), cb});
  } else {
    i = m_ref[id];
    m_heap[i].expires = Clock::now() + MS(timeOut);
    m_heap[i].cb = cb;
    if(!siftdown(i, m_heap.size())) siftup(i);
  }
}

void HeapTimer::adjust(int id, int timeOut) {
  assert(!m_heap.empty() && m_ref.count(id));
  m_heap[m_ref[id]].expires = Clock::now() + MS(timeOut);
  siftdown(m_ref[id], m_heap.size());
}

} // namespace webserver