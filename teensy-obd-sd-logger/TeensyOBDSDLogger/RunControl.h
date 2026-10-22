#pragma once
#include <stdint.h>
namespace automatic {
inline bool durationReached(uint32_t duration,uint32_t started,uint32_t now){return duration!=0&&uint32_t(now-started)>=duration;}
class RequestSpacing {
  uint32_t last=0;bool sent=false;
public:
  bool ready(uint32_t now)const{return !sent||uint32_t(now-last)>=10;}
  void submitted(uint32_t now){last=now;sent=true;}
};
// Debounce both edges; a button held at boot must be released before arming.
class Mode4Button {
  bool initialized=false,candidate=true,stable=true,armed=false,fired=false;
  uint32_t changedAt=0,pressedAt=0;
public:
  enum { Pressed=1, Released=2, Triggered=4 };
  unsigned update(bool rawPressed,uint32_t now){
    if(!initialized){initialized=true;candidate=rawPressed;changedAt=now;}
    if(rawPressed!=candidate){candidate=rawPressed;changedAt=now;}
    unsigned events=0;
    if(candidate!=stable&&uint32_t(now-changedAt)>=30){
      stable=candidate;
      if(!stable){armed=true;fired=false;events|=Released;}
      else if(armed){pressedAt=now;events|=Pressed;}
    }
    if(stable&&armed&&!fired&&uint32_t(now-pressedAt)>=2000){fired=true;events|=Triggered;}
    return events;
  }
  bool holding()const{return stable&&armed;}
};
class StopButton {
  bool held=false,fired=false,armed;uint32_t since=0,holdMs;
public:
  explicit StopButton(uint32_t hold=3000,bool requireRelease=false):armed(!requireRelease),holdMs(hold){}
  bool update(bool pressed,uint32_t now){
    if(!pressed){held=false;fired=false;armed=true;return false;}
    if(!armed)return false;
    if(!held){held=true;since=now;}
    if(fired||uint32_t(now-since)<holdMs)return false;
    fired=true;return true;
  }
};
}
