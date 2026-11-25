#include "../TeensyOBDSDLogger/Discovery.h"
#include <cassert>
#include <cstdio>
using automatic::Discovery;
int main(){
  Discovery silent;silent.begin();unsigned silentCount=0;
  while(silent.busy()){silent.finishTransaction();++silentCount;}assert(silentCount==8&&silent.count==0);
  Discovery negative;negative.begin();assert(negative.observe(0x7e8));assert(negative.support(0,1,12)==Discovery::Unknown);
  Discovery d;d.begin();uint8_t request[3];assert(d.request(request)==2&&request[0]==1&&request[1]==0);
  uint8_t a[]={0x41,0,0x80,0x10,0,1}; // PID 01, 0C and next-range support.
  assert(d.receive(0x7e8,a,sizeof a));
  uint8_t b[]={0x41,0,0,0,0,0};assert(d.receive(0x7e9,b,sizeof b));
  assert(d.support(0,1,1)==Discovery::Supported&&d.support(0,1,12)==Discovery::Supported);
  assert(d.support(0,1,2)==Discovery::Unsupported&&d.support(0,1,0x20)==Discovery::Supported);
  assert(d.support(0,1,0x21)==Discovery::Unknown&&d.support(1,1,0x21)==Discovery::Unsupported);
  d.finishTransaction();assert(d.attemptNumber()==2);d.finishTransaction();d.request(request);assert(request[1]==0x20);
  uint8_t wrong[]={0x41,0,0,0,0,0};assert(!d.receive(0x7e8,wrong,sizeof wrong));
  d.finishTransaction();assert(d.attemptNumber()==2);d.finishTransaction();d.request(request);assert(request[0]==2&&request[1]==0);
  assert(d.support(0,1,0x21)==Discovery::Unknown); // No response must not become unsupported.
  uint8_t freeze[]={0x42,0,0,0,0x10,0,0};assert(d.receive(0x7e8,freeze,sizeof freeze));
  assert(d.support(0,2,12)==Discovery::Supported&&d.support(0,2,12,1)==Discovery::Unknown);
  freeze[2]=1;assert(!d.receive(0x7e8,freeze,sizeof freeze));
  assert(d.support(1,2,12)==Discovery::Unknown&&d.support(0,3,0)==Discovery::NotApplicable);
  unsigned steps=0;while(d.busy()){d.finishTransaction();assert(++steps<20);}assert(d.count==2);
  uint32_t expected[8];uint8_t rpm[]={1,12};
  assert(d.expected(rpm,2,expected)==1&&expected[0]==0x7e8);
  uint8_t freezeRequest[]={2,12,0};assert(d.expected(freezeRequest,3,expected)==0);
  uint8_t dtc[]={3};assert(d.expected(dtc,1,expected)==0);
  d.begin();assert(d.expected(rpm,2,expected)==0);for(unsigned i=0;i<8;++i)assert(d.receive(0x18daf100+i,a,sizeof a));
  assert(!d.receive(0x18daf108,a,sizeof a)&&d.overflow&&d.count==8);
  // All ranges terminate without wrapping past E0; verify PID FF bit mapping.
  d.begin();steps=0;while(d.busy()){
    size_t n=d.request(request);uint8_t response[7]={};response[0]=request[0]+0x40;response[1]=request[1];
    unsigned offset=n==3?3:2;response[offset+3]=request[1]==0xE0?2:3;
    if(request[1]==0xE0){response[offset+3]=3;assert(!d.receive(0x7e8,response,offset+4));response[offset+3]=2;}
    assert(d.receive(0x7e8,response,offset+4));d.finishTransaction();assert(++steps<=64);
  }
  assert(d.support(0,1,0xff)==Discovery::Supported);
  // Fallback stops after Mode 01 and retains support from either ECU.
  Discovery live;
  live.begin(true);
  uint8_t first[]={0x41,0,0,0x10,0,0}; // PID 0C.
  uint8_t second[]={0x41,0,0,0x08,0,0}; // PID 0D.
  assert(live.receive(0x7e8,first,sizeof first));
  live.finishTransaction();
  assert(live.busy()&&live.request(request)==2&&request[0]==1);
  assert(live.receive(0x7e9,second,sizeof second));
  live.finishTransaction();
  assert(!live.busy());
  uint8_t speed[]={1,13}, unsupported[]={1,14};
  assert(live.requestSupport(rpm,2)==Discovery::Supported);
  assert(live.requestSupport(speed,2)==Discovery::Supported);
  assert(live.requestSupport(unsupported,2)==Discovery::Unsupported);
  puts("PASS: per-ECU discovery, continuation, bounded retries, unknown results, freeze frames and capacity");
}
