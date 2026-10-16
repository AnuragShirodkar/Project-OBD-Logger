#include "../TeensyOBDSDLogger/RunControl.h"
#include <cassert>
#include <cstdio>
int main(){
  using namespace automatic;
  assert(!durationReached(0,0,0xffffffffu));
  assert(!durationReached(60000,100,60099));assert(durationReached(60000,100,60100));
  assert(durationReached(1000,0xffffff00u,744));
  StopButton b;assert(!b.update(true,10));assert(!b.update(true,1009));assert(b.update(true,1010));
  assert(!b.update(false,1100));assert(!b.update(true,1200));assert(!b.update(false,1210));
  assert(!b.update(true,1220));assert(!b.update(true,2219));assert(b.update(true,2220));
  StopButton wrap;assert(!wrap.update(true,0xffffff00u));assert(wrap.update(true,744));
  puts("PASS: continuous/timed run and held-stop control, including clock wrap");
}
