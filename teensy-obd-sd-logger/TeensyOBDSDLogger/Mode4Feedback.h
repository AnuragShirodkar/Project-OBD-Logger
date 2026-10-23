#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace automatic {
// Independent of Arduino so confirmation and nonblocking patterns can be tested.
class Mode4Feedback {
public:
  enum Phase { Idle, Waiting, Success, Failure };
  Phase phase=Idle;
  bool active=false;
private:
  uint32_t ids[8]={}, at=0;
  size_t count=0;
  uint8_t confirmed=0;
  bool failed=false;
public:
  void queued(){phase=Waiting;}
  void begin(const uint32_t *expected,size_t n,bool inventoryComplete){
    count=n>8?8:n;confirmed=0;failed=!inventoryComplete||!n||n>8;
    for(size_t i=0;i<count;++i)ids[i]=expected[i];
    active=true;phase=Waiting;
  }
  void observe(const char *kind,uint32_t id,const uint8_t *p,size_t n){
    if(!active)return;
    if(!strcmp(kind,"POSITIVE")){
      if(n!=1||!p||p[0]!=0x44){failed=true;return;}
      for(size_t i=0;i<count;++i)if(ids[i]==id)confirmed|=uint8_t(1u<<i);
    }else if(strcmp(kind,"RESPONSE_PENDING"))failed=true;
  }
  bool finish(uint32_t now){
    bool ok=active&&!failed&&confirmed==uint8_t((1u<<count)-1);
    active=false;phase=ok?Success:Failure;at=now;return ok;
  }
  void update(uint32_t now){
    if((phase==Success&&uint32_t(now-at)>=3000)||(phase==Failure&&uint32_t(now-at)>=900))phase=Idle;
  }
  bool yellow(uint32_t now,bool normal)const{
    if(phase==Waiting)return (now/250)%2==0;
    if(phase==Success)return uint32_t(now-at)%1000<500;
    return normal;
  }
  bool red(uint32_t now,bool fault)const{
    return fault||(phase==Failure&&uint32_t(now-at)%300<150);
  }
  bool occupied()const{return phase!=Idle;}
};
}
