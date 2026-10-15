#include "../TeensyOBDTester/LogReport.h"
#include <assert.h>
#include <string>
#include <vector>
#include <stdio.h>
static std::vector<std::string> lines;
void line(const char *s){lines.push_back(s);}
bool contains(const char *s){for(const auto &l:lines)if(l.find(s)!=std::string::npos)return true;return false;}
int main(){
  char h[128],d[32];uint32_t t;
  obd::formatHeader(h,sizeof h,"");
  assert(std::string(h)=="OBD-SkillLogger Version 1.0.0\nDate:                    \n\n");
  size_t blankSize=strlen(h);
  assert(obd::parseLocalDate("2026-10-15 09:43:10",t));
  obd::formatLocalDate(t,d,sizeof d);assert(std::string(d)=="2026-10-15 09:43:10");
  obd::formatHeader(h,sizeof h,d);assert(strlen(h)==blankSize);
  assert(std::string(h)=="OBD-SkillLogger Version 1.0.0\nDate: 2026-10-15 09:43:10\n\n");
  for(const char *s:{"2026-02-29 00:00:00","2024-04-31 00:00:00","2026-10-15 24:00:00",
    "2026-10-15 09:60:00","2026-10-15 09:00:60","2026-1x-15 09:43:10","1999-12-31 23:59:59",
    "2026-10-15 09:43:10x","2026-00-15 09:43:10","2026-13-15 09:43:10"})
    assert(!obd::parseLocalDate(s,t));
  assert(obd::parseLocalDate("2024-02-29 23:59:59",t));
  obd::formatLocalDate(t+1,d,sizeof d);assert(std::string(d)=="2024-03-01 00:00:00");
  assert(obd::parseLocalDate("2000-01-01 00:00:00",t)&&t==0);
  assert(obd::parseLocalDate("2099-12-31 23:59:59",t));
  obd::formatLocalDate(t,d,sizeof d);assert(std::string(d)=="2099-12-31 23:59:59");
  uint8_t support[]={0x41,0,0,0x10,0,1};lines.clear();
  obd::reportPositive(line,0x7E8,support,sizeof support);
  assert(contains("0C")&&contains("20")&&contains("000007E8")&&contains("Engine RPM"));
  uint8_t rpm[]={0x41,0x0c,0x0c,0x80};lines.clear();
  obd::reportPositive(line,0x7E8,rpm,sizeof rpm);assert(contains("800.000 rpm"));
  uint8_t freeze[]={0x42,0x05,0,0};lines.clear();
  obd::reportPositive(line,0x7E8,freeze,sizeof freeze);assert(contains("-40.000 C")&&contains("Freeze frame: 00"));
  uint8_t voltage[]={0x41,0x42,0x38,0xb8};lines.clear();
  obd::reportPositive(line,0x7E8,voltage,sizeof voltage);assert(contains("14.520 V"));
  uint8_t dtc[]={0x43,2,0x01,0x10,0xc1,0x23};lines.clear();
  obd::reportPositive(line,0x7E8,dtc,sizeof dtc);assert(contains("P0110")&&contains("U0123"));
  dtc[1]=3;lines.clear();obd::reportPositive(line,0x7E8,dtc,sizeof dtc);
  assert(contains("Incomplete")&&!contains("No fault"));
  uint8_t empty[]={0x4a,0};lines.clear();obd::reportPositive(line,0x7E8,empty,2);assert(contains("No fault"));
  uint8_t vin[]={0x49,2,1,'T','E','S','T','0','0','0','0','0','0','0','0','0','0','0','0','1'};
  lines.clear();obd::reportPositive(line,0x7E8,vin,sizeof vin);assert(contains("TEST0000000000001"));
  lines.clear();obd::reportPositive(line,0x7E8,rpm,3);assert(contains("raw response")&&!contains("rpm"));
  char event[224];obd::formatEvent(event,sizeof event,12,1,500000,"NEGATIVE",0x7e8,"7F 01 12");
  assert(strstr(event,"NEGATIVE")&&strstr(event,"7F 01 12")&&strstr(event,"500000"));
  puts("PASS: exact header, blank/date layout, calendar validation, report decoding, malformed payloads");
}
