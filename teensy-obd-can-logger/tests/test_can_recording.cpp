#include "../TeensyOBDSDLogger/CanRecording.h"
#include <assert.h>
#include <string.h>
#include <fstream>
#include <iostream>

int main(int argc,char **argv){
  using automatic::PassiveAction;
  assert(automatic::passiveAction(false,false,100,0,599,500,0)==PassiveAction::Wait);
  assert(automatic::passiveAction(false,false,100,0,600,500,0)==PassiveAction::Start);
  assert(automatic::passiveAction(false,true,100,0,101,500,0)==PassiveAction::StopUser);
  assert(automatic::passiveAction(true,true,100,600,1600,500,1000)==PassiveAction::StopUser);
  assert(automatic::passiveAction(true,false,100,600,1599,500,1000)==PassiveAction::Wait);
  assert(automatic::passiveAction(true,false,100,600,1600,500,1000)==PassiveAction::StopDuration);
  assert(automatic::passiveAction(true,false,100,600,3600,500,0)==PassiveAction::Wait);
  assert(automatic::passiveAction(true,false,100,600,3601,500,0)==PassiveAction::StopIdle);
  assert(automatic::passiveAction(true,false,100,600,10000000,500,0,true,9999999)==PassiveAction::Wait);
  assert(automatic::passiveAction(true,false,0,100,6000,0,0,true,3000)==PassiveAction::Wait);
  assert(automatic::passiveAction(true,false,0,100,6001,0,0,true,3000)==PassiveAction::StopIdle);
  assert(automatic::passiveAction(true,false,0,100,6001,0,0,true,5999)==PassiveAction::Wait);
  assert(automatic::passiveAction(false,false,100,0,3100,5000,0)==PassiveAction::Wait);
  assert(automatic::passiveAction(true,false,0,0xfffffff0u,2984,0,0,true,0xfffffff0u)==PassiveAction::Wait);
  assert(automatic::passiveAction(true,false,0,0xfffffff0u,2985,0,0,true,0xfffffff0u)==PassiveAction::StopIdle);
  assert(automatic::passiveAction(true,false,0,0xfffffff0u,0x20u,0,48)==PassiveAction::StopDuration);
  automatic::CaptureClock clock;
  assert(clock.sample(0xfffffff0u)==0xfffffff0ull);
  assert(clock.sample(0x10u)==0x100000010ull);
  assert(clock.sample(0xfffffff0u)==0x1fffffff0ull);
  assert(clock.sample(0x20u)==0x200000020ull);
  assert(!automatic::canTrafficRecent(false,0,0));
  assert(automatic::canTrafficRecent(true,0xfffffff0u,10));
  assert(automatic::canTrafficRecent(true,100,3100));
  assert(!automatic::canTrafficRecent(true,100,3101));
  // Elapsed timestamp survives micros wrap and long silent gaps with loop sampling.
  automatic::CaptureClock silent;
  const auto start=silent.sample(100);
  silent.sample(2000000000u);silent.sample(4000000000u);
  assert(silent.sample(100)-start==0x100000000ull);

  char line[128];uint8_t data[]={0,1,2,3,0xAA,0xBB,0xCC,0xFF};
  assert(automatic::formatCanAsc(line,sizeof line,1234567,0x123,false,false,8,data));
  assert(!strcmp(line,"         1.234567 1  123          Rx   d   8 00 01 02 03 AA BB CC FF\n"));
  assert(automatic::formatCanAsc(line,sizeof line,4294967300ull,0x18DAF110,true,false,2,data));
  assert(!strcmp(line,"      4294.967300 1  18DAF110x    Rx   d   2 00 01\n"));
  assert(automatic::formatCanAsc(line,sizeof line,9,0x7ff,false,true,8,nullptr));
  assert(!strcmp(line,"         0.000009 1  7FF          Rx   r   8\n"));
  assert(automatic::formatCanAsc(line,sizeof line,0,0,false,false,0,nullptr));
  assert(!automatic::formatCanAsc(line,4,0,0,false,false,0,nullptr));
  assert(!automatic::formatCanAsc(line,sizeof line,0,0x800,false,false,0,nullptr));
  assert(!automatic::formatCanAsc(line,sizeof line,0,0x20000000,true,false,0,nullptr));
  assert(!automatic::formatCanAsc(line,sizeof line,0,1,false,false,9,data));
  assert(!automatic::formatCanAsc(line,sizeof line,0,1,false,false,1,nullptr));
  char header[768];
  assert(automatic::formatCanAscHeader(header,sizeof header,"1.2.0-dev4-idle-sd","OBD_1757338",
    "/LOGS/01_CAN_1757338.ASC",500000,0x1234ABCD));
  assert(strstr(header,"// Firmware: 1.2.0-dev4-idle-sd\n"));
  assert(strstr(header,"// SkiDrive OBD/CAN Logger\n// Product: SkiDrive\n"));
  assert(strstr(header,"// Logger: OBD_1757338\n"));
  assert(strstr(header,"// Config FNV1a: 1234ABCD\n"));
  assert(strstr(header,"// Firmware:")<strstr(header,"base hex"));
  assert(strstr(header,"// CAN channel: 1\nBegin Triggerblock\n"));
  assert(!strstr(header,"\ndate "));
  assert(!automatic::formatCanAscHeader(header,32,"test","OBD_1","example.asc",500000,0));
  assert(automatic::formatCanAscHeader(header,sizeof header,"1.2.0-dev4-idle-sd","OBD_1757338",
    "/LOGS/01_CAN_1757338.ASC",500000,0x1234ABCD));

  if(argc>1){
    std::ofstream fixture(argv[1]);assert(fixture);
    fixture<<"// Synthetic layout example; not a vehicle recording\n"<<header;
    const uint32_t ids[]={0x123,0x18DAF110,0x7ff,0};
    const uint64_t times[]={0,1234567,4294967300ull,4294967400ull};
    const uint8_t lengths[]={8,2,8,0};
    for(unsigned i=0;i<4;++i){
      assert(automatic::formatCanAsc(line,sizeof line,times[i],ids[i],i==1,i==2,lengths[i],data));
      fixture<<line;
    }
    fixture<<"// RXdrops=0 logDrops=0 hw_overruns=0\n"<<automatic::CAN_ASC_END;
    fixture.close();assert(fixture);
  }
  std::cout<<"CAN formatting, timestamps and activity tests passed\n";
}
