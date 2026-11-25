#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "RunControl.h"

namespace automatic {
enum class PassiveAction { Wait, Start, StopUser, StopDuration };
inline PassiveAction passiveAction(bool running,bool stop,uint32_t setup,uint32_t started,
                                  uint32_t now,uint32_t startup,uint32_t duration){
  if(stop)return PassiveAction::StopUser;
  if(!running&&uint32_t(now-setup)>=startup)return PassiveAction::Start;
  if(running&&durationReached(duration,started,now))return PassiveAction::StopDuration;
  return PassiveAction::Wait; // No bus-activity timeout: silence is not disconnection.
}
// Caller serializes ISR/main access, sampling at least once each micros() wrap.
class CaptureClock {
  uint32_t previous=0;
  uint64_t elapsed=0;
public:
  uint64_t sample(uint32_t now){elapsed+=uint32_t(now-previous);previous=now;return elapsed;}
};
inline bool canTrafficRecent(bool seen,uint32_t last,uint32_t now){
  return seen&&uint32_t(now-last)<5000;
}
inline bool formatCanAsc(char *out,size_t capacity,uint64_t us,uint32_t id,
                         bool extended,bool remote,uint8_t dlc,const uint8_t *data){
  if(dlc>8||id>(extended?0x1FFFFFFFu:0x7FFu)||(!remote&&dlc&&!data))return false;
  // Avoid host-specific printf 64-bit modifiers and floating-point rounding.
  char seconds[21];char *first=seconds+sizeof(seconds)-1;*first=0;
  uint64_t whole=us/1000000;
  do{*--first=char('0'+whole%10);whole/=10;}while(whole);
  int n=snprintf(out,capacity,"%s.%06lu 1 %lX%s Rx %c %u",
    first,(unsigned long)(us%1000000),
    (unsigned long)id,extended?"x":"",remote?'r':'d',unsigned(dlc));
  if(n<0||size_t(n)>=capacity)return false;
  size_t used=size_t(n);
  if(!remote)for(unsigned i=0;i<dlc;++i){
    n=snprintf(out+used,capacity-used," %02X",data[i]);
    if(n<0||size_t(n)>=capacity-used)return false;
    used+=size_t(n);
  }
  if(used+2>capacity)return false;
  out[used]='\n';out[used+1]=0;return true;
}
}
