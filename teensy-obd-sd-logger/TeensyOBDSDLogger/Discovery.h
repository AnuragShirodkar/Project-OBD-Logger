#pragma once
#include <stdint.h>
#include <stddef.h>
#include "J1979Validation.h"
namespace automatic {
// One boot-time scan, up to eight ECUs; Mode 02 maps are for freeze frame 00.
class Discovery {
public:
  enum Support { Unknown, Unsupported, Supported, NotApplicable };
  struct Ecu { uint32_t id=0;uint32_t maps[4][8]={};uint8_t known[4]={}; };
  Ecu ecus[8];size_t count=0;bool overflow=false;
private:
  unsigned modeIndex=0,range=0,attempt=0;bool active=false;
  static uint8_t modeAt(unsigned i){const uint8_t modes[]={1,2,6,9};return modes[i];}
  bool continuation()const{
    for(size_t i=0;i<count;++i)if((ecus[i].known[modeIndex]&(1u<<range))&&(ecus[i].maps[modeIndex][range]&1u))return true;
    return false;
  }
  bool missingExpected()const{
    if(range==0)return true; // Repeat base discovery once to catch a missed ECU.
    for(size_t i=0;i<count;++i){const auto &e=ecus[i];
      if((e.known[modeIndex]&(1u<<(range-1)))&&(e.maps[modeIndex][range-1]&1u)&&!(e.known[modeIndex]&(1u<<range)))return true;
    }
    return false;
  }
public:
  void begin(){*this=Discovery();active=true;}
  bool busy()const{return active;}
  unsigned attemptNumber()const{return attempt+1;}
  size_t request(uint8_t *p)const{p[0]=modeAt(modeIndex);p[1]=uint8_t(range*32);p[2]=0;return p[0]==2?3:2;}
  bool observe(uint32_t id){
    for(size_t i=0;i<count;++i)if(ecus[i].id==id)return true;
    if(count==8){overflow=true;return false;}
    ecus[count++].id=id;return true;
  }
  // Only feed validated, fully assembled positive responses from the active exchange.
  bool receive(uint32_t id,const uint8_t *p,size_t n){
    if(!active||!p)return false;
    unsigned offset=modeAt(modeIndex)==2?3:2;
    if(n!=offset+4||p[0]!=modeAt(modeIndex)+0x40||p[1]!=range*32||(offset==3&&p[2]!=0))return false;
    if(obd::invalidData(obd::validateData(p,n)))return false;
    if(!observe(id))return false;
    size_t i=0;for(;i<count;++i)if(ecus[i].id==id)break;
    uint32_t bits=(uint32_t(p[offset])<<24)|(uint32_t(p[offset+1])<<16)|(uint32_t(p[offset+2])<<8)|p[offset+3];
    // Preserve positive support evidence across retries; never erase it with a partial map.
    ecus[i].maps[modeIndex][range]|=bits;ecus[i].known[modeIndex]|=uint8_t(1u<<range);return true;
  }
  void finishTransaction(){
    if(!active)return;
    if(attempt==0&&missingExpected()){++attempt;return;}
    attempt=0;
    if(range<7&&continuation()){++range;return;}
    range=0;if(++modeIndex==4)active=false;
  }
  Support support(size_t ecu,uint8_t mode,uint8_t pid,uint8_t frame=0)const{
    unsigned m=0;for(;m<4&&modeAt(m)!=mode;++m){}
    if(m==4)return NotApplicable;
    if(ecu>=count||(mode==2&&frame!=0))return Unknown;
    const auto &e=ecus[ecu];
    if(pid==0)return (e.known[m]&1u)?Supported:Unknown;
    unsigned block=(unsigned(pid)-1)/32,bit=32-(unsigned(pid)-block*32);
    if(e.known[m]&(1u<<block))return (e.maps[m][block]&(uint32_t(1)<<bit))?Supported:Unsupported;
    // A known terminal range also rules out later identifiers.
    for(unsigned b=0;b<block;++b)if((e.known[m]&(1u<<b))&&!(e.maps[m][b]&1u))return Unsupported;
    return Unknown;
  }
  // Disable only with explicit negative bitmap evidence from EVERY observed ECU.
  // A silent ECU cannot be inventoried; this result applies to this boot's responders.
  Support requestSupport(const uint8_t *p,size_t n)const {
    if(!p||!n)return Unknown;
    if(p[0]==3||p[0]==7||p[0]==10)return NotApplicable;
    if(!((p[0]==1&&n==2)||(p[0]==2&&n==3)||((p[0]==6||p[0]==9)&&n==2)))return Unknown;
    if(active||overflow||!count)return Unknown;
    bool unknown=false;
    for(size_t i=0;i<count;++i){
      Support result=support(i,p[0],p[1],n>2?p[2]:0);
      if(result==Supported)return Supported;
      if(result!=Unsupported)unknown=true;
    }
    return unknown?Unknown:Unsupported;
  }
  // Early completion only when every discovered ECU has a known support answer.
  // Unknown support / no bitmap / overflow keeps the full collection window.
  size_t expected(const uint8_t *p,size_t n,uint32_t *ids)const {
    if(active||overflow||!count||n<2)return 0;
    size_t used=0;
    for(size_t i=0;i<count;++i){auto status=support(i,p[0],p[1],n>2?p[2]:0);
      if(status==Unknown||status==NotApplicable)return 0;
      if(status==Supported)ids[used++]=ecus[i].id;
    }return used;
  }
  static const char *name(Support s){switch(s){case Supported:return "SUPPORTED";case Unsupported:return "UNSUPPORTED";case NotApplicable:return "NO_PID_BITMAP";default:return "UNKNOWN";}}
};
}
