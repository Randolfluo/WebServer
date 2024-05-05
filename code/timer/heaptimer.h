#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <cstddef>
#include <queue>
#include <unordered_map>

#include <algorithm>
#include <chrono>
#include <functional>

#include <arpa/inet.h> //Internet地址转换函数
#include <assert.h>
#include <time.h>
#include <vector>

#include "../log/log.h"

using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;

struct TimerNode {
  int id;
  TimeStamp expires;
  TimeoutCallBack cb;
 
  bool operator<(const TimerNode &t) { return expires < t.expires; }
  bool operator>(const TimerNode &t) { return expires > t.expires; }
};

class HeapTimer {
public:
  HeapTimer();

  ~HeapTimer();

  void adjust(int id, int newExpires);

  void add(int id, int timeOus, const TimeoutCallBack &cb);

  void doWork(int id);

  void clear();

  void tick();

  void pop();

  int GetNextTick();

private:
  void del_(size_t i);

  void siftup_(size_t i);

  bool siftdown_(size_t index, size_t n);

  void SwapNode_(size_t i, size_t j);

  std::vector<TimerNode> heap_;

  std::unordered_map<int, size_t> ref_;
};

#endif