#pragma once
#include <atomic>
#include <stdint.h>
namespace automatic {
// Single ISR producer, single main-loop consumer. Publish slots after copying.
template<class T> class CaptureRing {
  T *slots=nullptr; uint32_t capacity=0;
  std::atomic<uint32_t> head{0},tail{0};
public:
  void begin(T *memory,uint32_t count){slots=memory;capacity=count;head.store(0);tail.store(0);}
  bool push(const T &value){
    uint32_t h=head.load(std::memory_order_relaxed);
    if(!capacity||h-tail.load(std::memory_order_acquire)>=capacity)return false;
    slots[h%capacity]=value;head.store(h+1,std::memory_order_release);return true;
  }
  bool pop(T &value){
    uint32_t t=tail.load(std::memory_order_relaxed);
    if(t==head.load(std::memory_order_acquire))return false;
    value=slots[t%capacity];tail.store(t+1,std::memory_order_release);return true;
  }
  uint32_t size() const {return head.load(std::memory_order_acquire)-tail.load(std::memory_order_acquire);}
};
}
