#include "../TeensyOBDSDLogger/Mode4Feedback.h"
#include <assert.h>
#include <stdio.h>
int main(){
  automatic::Mode4Feedback f;uint32_t ids[]={0x7e8,0x7e9};uint8_t ok[]={0x44},pending[]={0x7f,4,0x78},bad[]={0x44,0};
  f.queued();assert(f.occupied()&&f.yellow(0,false)&&!f.yellow(250,true));
  f.begin(ids,2,true);f.observe("POSITIVE",ids[0],ok,1);assert(!f.finish(0));
  assert(f.red(0,false)&&!f.red(150,false)&&f.red(300,false)&&f.red(600,false));
  f.update(900);assert(!f.occupied());
  f.begin(ids,2,true);f.observe("RESPONSE_PENDING",ids[1],pending,3);
  f.observe("POSITIVE",ids[0],ok,1);f.observe("POSITIVE",ids[0],ok,1);f.observe("POSITIVE",ids[1],ok,1);
  assert(f.finish(100));assert(f.yellow(100,false)&&!f.yellow(600,true)&&f.yellow(1100,false)&&f.yellow(2100,false));
  f.update(3100);assert(!f.occupied()&&f.yellow(3100,true));
  f.begin(ids,1,true);f.observe("NEGATIVE",ids[1],pending,3);f.observe("POSITIVE",ids[0],ok,1);assert(!f.finish(0));
  f.begin(ids,1,true);f.observe("POSITIVE",ids[0],bad,2);assert(!f.finish(0));
  f.begin(ids,1,true);f.observe("POSITIVE",ids[1],ok,1);assert(!f.finish(0));
  f.begin(ids,1,false);f.observe("POSITIVE",ids[0],ok,1);assert(!f.finish(0));
  f.begin(ids,0,true);f.observe("POSITIVE",ids[0],ok,1);assert(!f.finish(0));
  f.begin(ids,1,true);f.observe("POSITIVE",ids[0],ok,1);f.observe("CANCELLED",0,nullptr,0);assert(!f.finish(0));
  f.begin(ids,1,true);f.observe("POSITIVE",ids[0],ok,1);assert(f.finish(0xffffff00u));f.update(uint32_t(0xffffff00u+3000u));assert(!f.occupied());
  assert(f.red(0,true));puts("Mode 04 feedback tests passed");
}
