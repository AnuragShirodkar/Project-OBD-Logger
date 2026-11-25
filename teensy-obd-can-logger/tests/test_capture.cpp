#include "../TeensyOBDSDLogger/CaptureRing.h"
#include <cassert>
#include <cstdio>
struct Frame {unsigned sequence;unsigned data[8];};
int main(){
  Frame memory[64];automatic::CaptureRing<Frame> q;q.begin(memory,64);Frame out{};
  assert(!q.pop(out));
  // Model SD stalls, full queue, explicit overflow, then drain without resetting.
  for(unsigned round=0;round<1000;++round){
    for(unsigned i=0;i<64;++i){Frame f{};f.sequence=round*64+i;f.data[7]=~f.sequence;assert(q.push(f));}
    assert(q.size()==64);assert(!q.push(out));
    for(unsigned i=0;i<64;++i){assert(q.pop(out));assert(out.sequence==round*64+i);assert(out.data[7]==~out.sequence);}
    assert(!q.pop(out));assert(q.size()==0);
  }
  // Wrap buffer slots while producer and consumer make progress at different rates.
  unsigned next=0;
  for(unsigned i=0;i<10000;++i){Frame f{};f.sequence=i;assert(q.push(f));if(i%3==2){for(unsigned j=0;j<3;++j){assert(q.pop(out));assert(out.sequence==next++);}}}
  while(q.pop(out))assert(out.sequence==next++);
  assert(next==10000);puts("PASS: capture ordering, capacity, overflow, slot wrap and shutdown drain");
}
