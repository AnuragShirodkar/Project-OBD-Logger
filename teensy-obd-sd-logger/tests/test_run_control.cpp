#include "../TeensyOBDSDLogger/RunControl.h"
#include <cassert>
#include <cstdio>
int main(){
  using namespace automatic;
  assert(!durationReached(0,0,0xffffffffu));
  assert(!durationReached(60000,100,60099));assert(durationReached(60000,100,60100));
  assert(durationReached(1000,0xffffff00u,744));
  StopButton b;
  assert(!b.update(true,10));assert(!b.update(true,3009));assert(b.update(true,3010));
  assert(!b.update(true,9000)); // Holding must not repeat.
  assert(!b.update(false,9100));assert(!b.update(true,9200));assert(!b.update(false,9210));
  assert(!b.update(true,9220));assert(!b.update(true,12219));assert(b.update(true,12220));
  StopButton wrap;assert(!wrap.update(true,0xffffff00u));assert(wrap.update(true,2744));
  StopButton mode4(2000,true);
  assert(!mode4.update(true,0));assert(!mode4.update(true,5000)); // Held at boot is ignored.
  assert(!mode4.update(false,5010));assert(!mode4.update(true,5020));
  assert(!mode4.update(true,7019));assert(mode4.update(true,7020));
  assert(!mode4.update(true,10000));assert(!mode4.update(false,10001));
  assert(!mode4.update(true,10002));assert(mode4.update(true,12002));
  puts("PASS: continuous/timed run, 3-second stop, 2-second manual button, release and clock wrap");
}
