#include "../TeensyOBDSDLogger/Readiness.h"
#include <cassert>
#include <cstring>
#include <string>
#include <cstdio>
std::string trace;
void line(const char *s){trace+=s;}
int main() {
  obd::Readiness r;
  const uint8_t spark[]={0x41,1,0x82,7,0x65,0x21};
  assert(obd::decodeReadiness(spark,6,r));
  assert(r.mil()&&r.dtcs()==2&&!r.compression()&&!r.reservedSet());
  assert(r.commonSupported(0)&&r.commonSupported(1)&&r.commonSupported(2));
  assert(r.commonComplete(0)&&r.commonComplete(1)&&r.commonComplete(2));
  assert(!strcmp(r.monitorState(0),"INCOMPLETE"));
  assert(!strcmp(r.monitorState(1),"N/A"));
  assert(!strcmp(r.monitorState(2),"COMPLETE"));
  assert(!strcmp(r.monitorState(5),"INCOMPLETE"));
  assert(!strcmp(r.monitorState(6),"COMPLETE"));
  obd::traceReadiness(line,0x7E8,spark,6);
  assert(trace.find("MIL commanded=ON; confirmed emissions DTC count=2")!=std::string::npos);
  assert(trace.find("Oxygen sensor heater: COMPLETE")!=std::string::npos);
  assert(trace.find("PM filter")==std::string::npos);
  const uint8_t diesel[]={0x41,1,0,0x0F,0x6B,2};
  assert(obd::decodeReadiness(diesel,6,r)&&r.compression()&&!r.mil()&&r.dtcs()==0);
  assert(!strcmp(r.monitorName(0),"NMHC catalyst"));
  assert(!strcmp(r.monitorName(1),"NOx/SCR aftertreatment"));
  assert(!strcmp(r.monitorState(1),"INCOMPLETE"));
  assert(!strcmp(r.monitorName(3),"Boost pressure"));
  assert(!strcmp(r.monitorName(5),"Exhaust gas sensor"));
  assert(!strcmp(r.monitorName(6),"PM filter"));
  assert(r.monitorName(2)==nullptr&&r.monitorName(4)==nullptr);
  trace.clear();obd::traceReadiness(line,0x7E9,diesel,6);
  assert(trace.find("ECU 7E9")!=std::string::npos);
  assert(trace.find("PM filter: COMPLETE")!=std::string::npos);
  assert(trace.find("Oxygen sensor")==std::string::npos);
  // Each ECU is decoded independently; a diesel reply cannot affect a later spark reply.
  trace.clear();obd::traceReadiness(line,0x7E8,spark,6);
  assert(trace.find("Heated catalyst")!=std::string::npos&&trace.find("PM filter")==std::string::npos);
  uint8_t unsupported[]={0x41,1,0,0x04,0,0xEF};
  assert(obd::decodeReadiness(unsupported,6,r));
  for(unsigned bit=0;bit<8;++bit)assert(!strcmp(r.monitorState(bit),"N/A"));
  assert(!r.commonSupported(0)&&r.commonComplete(0));
  unsupported[3]=0x14;assert(obd::decodeReadiness(unsupported,6,r)&&!r.commonComplete(0));
  // Reserved bits are warned about; they do not become invented monitor labels.
  uint8_t reserved[]={0x41,1,0,0x88,0x14,0x14};
  trace.clear();obd::traceReadiness(line,0x7E8,reserved,6);
  assert(trace.find("READINESS_WARNING")!=std::string::npos);
  assert(trace.find("Evaporative system")==std::string::npos);
  const uint8_t cycle[]={0x41,0x41,0,0,0,0};
  const uint8_t freeze[]={0x42,1,0,0,0,0,0};
  trace.clear();obd::traceReadiness(line,0x7E8,cycle,6);
  obd::traceReadiness(line,0x7E8,freeze,7);obd::traceReadiness(line,0x7E8,spark,5);
  assert(trace.empty()&&!obd::decodeReadiness(nullptr,6,r));
  assert(!r.mil()&&r.dtcs()==0); // Failed decode clears stale output.
  puts("PASS: per-ECU readiness, spark/diesel labels, MIL/count, unsupported monitors, reserved warnings and exact payload scope");
}
