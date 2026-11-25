#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "RunControl.h"

namespace automatic {
constexpr const char CAN_ASC_END[]="End Triggerblock\n";
constexpr size_t CAN_ASC_END_SIZE=sizeof(CAN_ASC_END)-1;
inline bool formatCanAscHeader(char *out,size_t capacity,const char *firmware,
                               const char *logger,const char *file,uint32_t bitrate,uint32_t fingerprint){
  const int n=snprintf(out,capacity,
    "// SkiDrive Classical CAN capture\n// Product: SkiDrive\n"
    "// Firmware: %s\n// Logger: %s\n// File: %s\n"
    "// Operating mode: can; bitrate: %lu; listen-only; standard and extended IDs\n"
    "// Config FNV1a: %08lX\n"
    "// Time: elapsed seconds since capture start; software receive time; calendar time unavailable\n"
    "// Remote frames retain DLC but contain no payload bytes\n"
    "base hex timestamps absolute\nno internal events logged\n"
    "// CAN channel: 1\nBegin Triggerblock\n"
    "// ;    time  can ident           attr dlc data ...\n",
    firmware,logger,file,(unsigned long)bitrate,(unsigned long)fingerprint);
  return n>=0&&size_t(n)<capacity;
}
constexpr uint32_t CAN_IDLE_TIMEOUT_MS=3000;
inline bool canIdleExpired(bool seen,uint32_t last,uint32_t started,uint32_t now){
  return uint32_t(now-(seen?last:started))>CAN_IDLE_TIMEOUT_MS;
}
enum class PassiveAction { Wait, Start, StopUser, StopDuration, StopIdle };
inline PassiveAction passiveAction(bool running,bool stop,uint32_t setup,uint32_t started,
                                  uint32_t now,uint32_t startup,uint32_t duration,
                                  bool seen=false,uint32_t last=0){
  if(stop)return PassiveAction::StopUser;
  if(!running&&uint32_t(now-setup)>=startup)return PassiveAction::Start;
  if(running&&durationReached(duration,started,now))return PassiveAction::StopDuration;
  if(running&&canIdleExpired(seen,last,started,now))return PassiveAction::StopIdle;
  return PassiveAction::Wait;
}
// Caller serializes ISR/main access, sampling at least once each micros() wrap.
class CaptureClock {
  uint32_t previous=0;
  uint64_t elapsed=0;
public:
  uint64_t sample(uint32_t now){elapsed+=uint32_t(now-previous);previous=now;return elapsed;}
};
inline bool canTrafficRecent(bool seen,uint32_t last,uint32_t now){
  return seen&&uint32_t(now-last)<=CAN_IDLE_TIMEOUT_MS;
}
inline bool formatCanAsc(char *out,size_t capacity,uint64_t us,uint32_t id,
                         bool extended,bool remote,uint8_t dlc,const uint8_t *data){
  if(dlc>8||id>(extended?0x1FFFFFFFu:0x7FFu)||(!remote&&dlc&&!data))return false;
  // Avoid host-specific printf 64-bit modifiers and floating-point rounding.
  char seconds[21];char *first=seconds+sizeof(seconds)-1;*first=0;
  uint64_t whole=us/1000000;
  do{*--first=char('0'+whole%10);whole/=10;}while(whole);
  char timestamp[30],identifier[10];
  snprintf(timestamp,sizeof timestamp,"%s.%06lu",first,(unsigned long)(us%1000000));
  snprintf(identifier,sizeof identifier,"%lX%s",(unsigned long)id,extended?"x":"");
  int n=snprintf(out,capacity,"%17s 1  %-12s Rx   %c   %u",
    timestamp,identifier,remote?'r':'d',unsigned(dlc));
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
