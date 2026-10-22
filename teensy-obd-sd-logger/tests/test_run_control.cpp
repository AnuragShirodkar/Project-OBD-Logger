#include "../TeensyOBDSDLogger/RunControl.h"
#include <cassert>
#include <cstdio>
int main(){
  using namespace automatic;
  assert(!durationReached(0,0,0xffffffffu));
  assert(!durationReached(60000,100,60099));assert(durationReached(60000,100,60100));
  assert(durationReached(1000,0xffffff00u,744));
  RequestSpacing spacing;assert(spacing.ready(0));spacing.submitted(100);
  assert(!spacing.ready(108));assert(spacing.ready(110));assert(spacing.ready(125));
  spacing.submitted(0xfffffffau);assert(!spacing.ready(3));assert(spacing.ready(4));
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
  Mode4Button manual;
  assert(manual.update(true,0)==0&&manual.update(true,5000)==0); // No clear at boot.
  assert(manual.update(false,5010)==0);
  assert(manual.update(false,5040)==Mode4Button::Released);
  assert(manual.update(true,5050)==0);assert(manual.update(false,5055)==0); // Press bounce.
  assert(manual.update(true,5060)==0);assert(manual.update(true,5090)==Mode4Button::Pressed);
  assert(manual.holding());assert(manual.update(true,7089)==0);
  assert(manual.update(true,7090)==Mode4Button::Triggered);
  assert(manual.update(true,9000)==0);
  assert(manual.update(false,9001)==0);assert(manual.update(true,9010)==0); // Release bounce cannot rearm.
  assert(manual.update(true,12000)==0);
  assert(manual.update(false,12001)==0);assert(manual.update(false,12031)==Mode4Button::Released);
  assert(!manual.holding());assert(manual.update(true,12040)==0);assert(manual.update(true,12070)==Mode4Button::Pressed);
  assert(manual.update(true,14070)==Mode4Button::Triggered);
  Mode4Button wrapped;wrapped.update(false,0xfffff000u);wrapped.update(false,0xfffff020u);
  wrapped.update(true,0xffffff00u);assert(wrapped.update(true,0xffffff1eu)==Mode4Button::Pressed);
  assert(wrapped.update(true,1773)==0);assert(wrapped.update(true,1774)==Mode4Button::Triggered);
  puts("PASS: continuous/timed run, 3-second stop, 2-second manual button, release and clock wrap");
}
