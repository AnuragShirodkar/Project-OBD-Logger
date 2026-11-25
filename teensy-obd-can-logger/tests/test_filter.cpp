#include "../TeensyOBDSDLogger/Discovery.h"
#include "../TeensyOBDSDLogger/Schedule.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
using namespace automatic;
int main(){
  uint8_t rpm[]={1,12},fuel[]={1,0x2f},freeze[]={2,12,0},otherFrame[]={2,12,1},monitor[]={6,1},vin[]={9,2};
  Discovery d;assert(d.requestSupport(rpm,2)==Discovery::Unknown);
  d.begin();uint8_t map[]={0x41,0,0,0x10,0,0};assert(d.receive(0x7e8,map,6));
  assert(d.requestSupport(fuel,2)==Discovery::Unknown); // Never filter an unfinished scan.
  while(d.busy())d.finishTransaction();
  assert(d.requestSupport(rpm,2)==Discovery::Supported);
  assert(d.requestSupport(fuel,2)==Discovery::Unsupported); // Terminal base range.
  assert(d.requestSupport(freeze,3)==Discovery::Unknown);
  assert(d.requestSupport(monitor,2)==Discovery::Unknown);
  assert(d.requestSupport(vin,2)==Discovery::Unknown);
  d.ecus[0].known[1]=1;d.ecus[0].known[2]=1;d.ecus[0].known[3]=1;
  assert(d.requestSupport(freeze,3)==Discovery::Unsupported);
  assert(d.requestSupport(otherFrame,3)==Discovery::Unknown);
  assert(d.requestSupport(monitor,2)==Discovery::Unsupported);
  assert(d.requestSupport(vin,2)==Discovery::Unsupported);
  for(uint8_t mode: {uint8_t(3),uint8_t(7),uint8_t(10)})assert(d.requestSupport(&mode,1)==Discovery::NotApplicable);
  assert(d.observe(0x7e9)); // Missing maps cannot suppress a request.
  assert(d.requestSupport(fuel,2)==Discovery::Unknown);
  assert(d.requestSupport(rpm,2)==Discovery::Supported);
  d.ecus[1].known[0]=1;assert(d.requestSupport(fuel,2)==Discovery::Unsupported);
  d.ecus[1].maps[0][0]=1;assert(d.requestSupport(fuel,2)==Discovery::Unknown); // Advertised range missing.
  d.ecus[1].known[0]|=2;d.ecus[1].maps[0][1]=1u<<17;
  assert(d.requestSupport(fuel,2)==Discovery::Supported);
  d.overflow=true;assert(d.requestSupport(fuel,2)==Discovery::Unknown);
  d.begin();assert(d.count==0&&d.requestSupport(fuel,2)==Discovery::Unknown); // Boot clears previous vehicle.

  Job jobs[3];jobs[0].enabled=false;jobs[2].enabled=false;Schedule schedule;
  schedule.begin(jobs,3,0);assert(schedule.select(jobs,3,100)==1);
  assert(schedule.select(jobs,3,100)==-1);schedule.finish(jobs,3,1000);
  assert(jobs[0].skipped==0&&jobs[2].skipped==0&&jobs[0].late==0);
  jobs[1].enabled=false;assert(schedule.select(jobs,3,2000)==-1);
  assert(schedule.select(jobs,0,2000)==-1);
  jobs[1].enabled=true;schedule.begin(jobs,3,0xfffffffau);
  assert(schedule.select(jobs,3,4)==1);
  puts("PASS: conservative support filtering, unknown/mixed ECUs, modes, disabled scheduling and wrap");
}
