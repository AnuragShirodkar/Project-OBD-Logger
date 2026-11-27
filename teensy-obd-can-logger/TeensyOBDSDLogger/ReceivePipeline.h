#pragma once
#include "CaptureRing.h"
namespace automatic {
// ISR -> internal staging -> main-loop external backlog -> main-loop decoder.
// Only stage() is called by the interrupt. transfer()/pop() have one main caller.
template<class T> class ReceivePipeline {
  CaptureRing<T> staging,backlog;
  uint32_t backlogCapacity=0;
public:
  void begin(T *internal,uint32_t internalCount,T *external,uint32_t externalCount){
    staging.begin(internal,internalCount);backlog.begin(external,externalCount);
    backlogCapacity=externalCount;
  }
  bool stage(const T &value){return staging.push(value);}
  uint32_t staged() const{return staging.size();}
  uint32_t buffered() const{return backlog.size();}
  uint32_t pending() const{return staged()+buffered();}
  uint32_t transfer(uint32_t limit){
    T value;uint32_t moved=0;
    while(moved<limit&&backlog.size()<backlogCapacity&&staging.pop(value)){
      // Main is the only backlog producer/consumer; the checked slot stays free.
      backlog.push(value);++moved;
    }
    return moved;
  }
  bool pop(T &value){return backlog.pop(value);}
};
}
