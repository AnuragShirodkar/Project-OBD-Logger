#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace obd {
// Mode 01 PID 01 only: OCT2011 Table B2, pages 6-8.
// Mode 01 PID 41 has different semantics and must not use this decoder.
struct Readiness {
  uint8_t a=0,b=0,c=0,d=0;
  bool mil()const{return (a&0x80)!=0;}
  unsigned dtcs()const{return a&0x7F;}
  bool compression()const{return (b&8)!=0;}
  bool commonSupported(unsigned bit)const{return bit<3&&(b&(1u<<bit));}
  bool commonComplete(unsigned bit)const{return bit<3&&!(b&(1u<<(bit+4)));}
  uint8_t reservedMask()const{return compression()?0x14:0x10;}
  bool reservedSet()const{return (b&0x80)||((c|d)&reservedMask());}
  bool monitorSupported(unsigned bit)const{return bit<8&&!(reservedMask()&(1u<<bit))&&(c&(1u<<bit));}
  const char *monitorState(unsigned bit)const {
    if(!monitorSupported(bit))return "N/A";
    return d&(1u<<bit)?"INCOMPLETE":"COMPLETE";
  }
  const char *monitorName(unsigned bit)const {
    if(bit>=8||(reservedMask()&(1u<<bit)))return nullptr;
    switch(bit) {
      case 0:return compression()?"NMHC catalyst":"Catalyst";
      case 1:return compression()?"NOx/SCR aftertreatment":"Heated catalyst";
      case 2:return "Evaporative system";
      case 3:return compression()?"Boost pressure":"Secondary air";
      case 5:return compression()?"Exhaust gas sensor":"Oxygen sensor";
      case 6:return compression()?"PM filter":"Oxygen sensor heater";
      case 7:return "EGR/VVT";
      default:return nullptr;
    }
  }
};
inline bool decodeReadiness(const uint8_t *p,size_t n,Readiness &out) {
  out=Readiness();
  if(!p||n!=6||p[0]!=0x41||p[1]!=1)return false;
  out.a=p[2];out.b=p[3];out.c=p[4];out.d=p[5];return true;
}
typedef void (*ReadinessLine)(const char *);
inline void traceReadiness(ReadinessLine emit,uint32_t id,const uint8_t *p,size_t n) {
  Readiness r;if(!emit||!decodeReadiness(p,n,r))return;
  char line[180];
  snprintf(line,sizeof line,"  READINESS ECU %03lX: MIL commanded=%s; confirmed emissions DTC count=%u\n",
           (unsigned long)id,r.mil()?"ON":"OFF",r.dtcs());emit(line);
  const char *common[]={"Misfire","Fuel system","Comprehensive components"};
  for(unsigned i=0;i<3;++i) {
    snprintf(line,sizeof line,"    %s: supported=%s; ready=%s\n",common[i],
             r.commonSupported(i)?"YES":"NO",r.commonComplete(i)?"YES":"NO");emit(line);
  }
  for(unsigned bit=0;bit<8;++bit) {
    const char *name=r.monitorName(bit);if(!name)continue;
    snprintf(line,sizeof line,"    %s: %s\n",name,r.monitorState(bit));emit(line);
  }
  if(r.reservedSet())emit("    READINESS_WARNING: reserved bits set; raw response retained\n");
  emit("    Readiness is completion since DTC clearing, not a test-pass result.\n");
}
} // namespace obd
