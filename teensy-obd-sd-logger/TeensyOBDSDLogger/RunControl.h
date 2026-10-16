#pragma once
#include <stdint.h>
namespace automatic {
inline bool durationReached(uint32_t duration,uint32_t started,uint32_t now){return duration!=0&&uint32_t(now-started)>=duration;}
class StopButton {
  bool held=false;uint32_t since=0;
public:
  bool update(bool pressed,uint32_t now){
    if(!pressed){held=false;return false;}
    if(!held){held=true;since=now;}
    return uint32_t(now-since)>=1000;
  }
};
}
