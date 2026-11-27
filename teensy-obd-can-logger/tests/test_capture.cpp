#include "../TeensyOBDSDLogger/CaptureRing.h"
#include "../TeensyOBDSDLogger/ReceivePipeline.h"
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
  assert(next==10000);
  Frame internal[8],external[16];automatic::ReceivePipeline<Frame> pipeline;
  pipeline.begin(internal,8,external,16);
  // Fill external backlog, then interrupt staging. A blocked transfer must not
  // discard an already captured record or let a later frame pass it.
  for(unsigned i=0;i<24;++i){
    Frame f{};f.sequence=i;f.data[7]=~i;assert(pipeline.stage(f));
    pipeline.transfer(3);
  }
  assert(pipeline.buffered()==16&&pipeline.staged()==8&&pipeline.pending()==24);
  assert(pipeline.transfer(100)==0);assert(!pipeline.stage(out));
  for(unsigned i=0;i<24;++i){
    pipeline.transfer(2);assert(pipeline.pop(out));
    assert(out.sequence==i&&out.data[7]==~i);
  }
  assert(pipeline.pending()==0&&!pipeline.pop(out));
  // Uneven burst production, bounded transfers and slow draining model repeated
  // writes, wrap staging slots, and exercise shutdown with staging still pending.
  unsigned produced=0,consumed=0;
  for(unsigned tick=0;tick<100000;++tick){
    for(unsigned j=0;j<3;++j){Frame f{};f.sequence=produced;f.data[7]=~produced;
      if(pipeline.stage(f))++produced;else break;}
    pipeline.transfer(tick%5);
    if(tick%3==0&&pipeline.pop(out)){
      assert(out.sequence==consumed&&out.data[7]==~consumed);++consumed;
    }
  }
  while(pipeline.pending()){
    pipeline.transfer(1);
    if(pipeline.pop(out)){assert(out.sequence==consumed&&out.data[7]==~consumed);++consumed;}
  }
  assert(consumed==produced);
  puts("PASS: capture/staging ordering, saturation, blocked transfer, wrap and complete shutdown drain");
}
