#include "../TeensyOBDSDLogger/DiagnosticCore.h"
#include <assert.h>
#include <vector>
#include <string>
#include <stdio.h>
struct Event {std::string kind;uint32_t id;std::vector<uint8_t> data;};
struct Fake:obd::Sink {
  std::vector<obd::Frame> sent;std::vector<Event> events;bool allow=true;
  bool send(const obd::Frame &f)override{if(!allow)return false;sent.push_back(f);return true;}
  void event(const char*k,uint32_t id,const uint8_t*p,size_t n)override{
    if (!strcmp(k,"UNVERIFIED_DEFINITION")) return; // Separately covered by data-validation tests.
    events.push_back({k,id,n?std::vector<uint8_t>(p,p+n):std::vector<uint8_t>()});
  }
  bool saw(const char*k){for(auto &e:events)if(e.kind==k)return true;return false;}
};
obd::Frame frame(uint32_t id,std::initializer_list<uint8_t> p,bool ext=false){
  obd::Frame f={};f.id=id;f.extended=ext;f.len=uint8_t(p.size());size_t i=0;
  for(auto b:p) { f.data[i++]=b; } return f;
}
int main(){
  // Manual clear request: exact wire payload, serialized operation, positive/negative/timeout.
  {
    static Fake clearSink;static obd::DiagnosticCore clearCore(clearSink);obd::Address address;
    const uint8_t clear[]={4};
    assert(clearCore.start(clear,1,address,0));
    const auto &tx=clearSink.sent.back();assert(tx.id==0x7df&&tx.len==8&&tx.data[0]==1&&tx.data[1]==4);
    for(unsigned i=2;i<8;++i)assert(tx.data[i]==0);
    assert(!clearCore.start(clear,1,address,1));
    clearCore.receive(frame(0x7e8,{1,0x44,0,0,0,0,0,0}),10);
    assert(clearSink.saw("POSITIVE"));clearCore.tick(1510);assert(!clearCore.busy());
    clearSink.events.clear();assert(clearCore.start(clear,1,address,2000));
    clearCore.receive(frame(0x7e8,{3,0x7f,4,0x22,0,0,0,0}),2010);
    assert(clearSink.saw("NEGATIVE"));clearCore.tick(3510);assert(!clearCore.busy());
    clearSink.events.clear();assert(clearCore.start(clear,1,address,4000));
    clearCore.tick(5500);assert(clearSink.saw("NO_RESPONSE_TIMEOUT")&&!clearCore.busy());
  }
  uint8_t p[7];size_t n=0;
  assert(obd::parseHex("01 0c",p,7,n)&&n==2&&p[1]==12);
  for(const char *s:{"","01 ZZ","01 100","01 02 03 04 05 06 07 08","01\r","0x01"})
    assert(!obd::parseHex(s,p,7,n));
  static Fake f;static obd::DiagnosticCore c(f);obd::Address a;
  uint8_t rpm[]={1,12};
  assert(c.start(rpm,2,a,0));assert(f.sent[0].id==0x7df&&f.sent[0].data[0]==2);
  assert(!c.start(rpm,2,a,0));
  c.receive(frame(0x7e8,{4,0x41,0x0d,0,1}),10);assert(!f.saw("POSITIVE"));
  c.receive(frame(0x7e8,{4,0x41,0x0c,0x0c,0x80}),20);
  c.receive(frame(0x7e9,{3,0x7f,1,0x12}),30);
  assert(f.saw("POSITIVE")&&f.saw("NEGATIVE"));
  c.tick(1530);assert(!c.busy()&&!f.saw("NO_RESPONSE_TIMEOUT"));
  f.events.clear();assert(c.start(rpm,2,a,2000));
  c.receive(frame(0x7e8,{4,0x41,0x0c,0x0c,0x80}),1999);
  c.tick(3500);assert(f.saw("NO_RESPONSE_TIMEOUT"));
  f.events.clear();assert(c.start(rpm,2,a,4000));
  c.receive(frame(0x7e8,{3,0x7f,1,0x78}),4010);c.tick(6000);
  assert(c.busy()&&f.saw("RESPONSE_PENDING"));c.tick(14000);
  assert(!c.busy()&&f.saw("RESPONSE_DEADLINE"));
  // Reassembly with sequence wrap and multiple block-size handshakes.
  f.events.clear();f.sent.clear();uint8_t vin[]={9,4} /* synthetic transport data, not a VIN */;assert(c.start(vin,2,a,15000));
  std::vector<uint8_t> data(130);data[0]=0x49;data[1]=4;
  for(size_t i=2;i<data.size();++i)data[i]=uint8_t(i);
  auto ff=frame(0x7e8,{0x10,130,0x49,4,2,3,4,5});c.receive(ff,15010);
  assert(f.sent.size()==2&&f.sent[1].id==0x7e0&&f.sent[1].data[0]==0x30);
  size_t have=6;unsigned seq=1;uint32_t t=15020;
  while(have<data.size()){
    auto cf=frame(0x7e8,{uint8_t(0x20|(seq&15)),0,0,0,0,0,0,0});
    for(size_t i=1;i<8&&have<data.size();++i)cf.data[i]=data[have++];
    c.receive(cf,t);t+=10;++seq;
  }
  assert(f.events.back().kind=="POSITIVE"&&f.events.back().data==data);
  assert(f.sent.size()==4);c.tick(17000);
  // Distinct ECU reassembly and extended physical flow-control mapping.
  a.extended=true;a.tx=0x18db33f1;f.sent.clear();f.events.clear();
  assert(c.start(vin,2,a,18000));ff.extended=true;ff.id=0x18daf110;
  c.receive(ff,18001);assert(f.sent.back().id==0x18da10f1);
  c.receive(frame(0x18daf110,{0x22,6,7,8,9,10,11,12},true),18010);
  assert(f.saw("ISOTP_SEQUENCE_ERROR"));c.tick(19501);
  a.extended=false;a.tx=0x700;a.rx=0x708;f.sent.clear();f.events.clear();
  assert(c.start(vin,2,a,20000));ff.extended=false;ff.id=0x708;
  c.receive(ff,20001);assert(f.sent.back().id==0x700);
  c.tick(21001);assert(f.saw("ISOTP_TIMEOUT"));c.cancel();
  // Unsigned time rollover and transmit failure.
  a=obd::Address();f.events.clear();assert(c.start(rpm,2,a,0xfffffff0));
  c.tick(1500);assert(!c.busy()&&f.saw("NO_RESPONSE_TIMEOUT"));
  f.allow=false;f.events.clear();assert(!c.start(rpm,2,a,0));
  assert(f.saw("REQUEST_TX_FAILED"));
  f.allow=true;f.events.clear();f.sent.clear();a=obd::Address();
  assert(c.start(vin,2,a,10000));
  c.receive(frame(0x7e8,{0x10,10,0x49,4,1,65,66,67}),10001);
  c.receive(frame(0x7e9,{0x10,10,0x49,4,1,97,98,99}),10002);
  assert(f.sent.size()==3&&f.sent[1].id==0x7e0&&f.sent[2].id==0x7e1);
  c.receive(frame(0x7e9,{0x21,100,101,102,103}),10010);
  c.receive(frame(0x7e8,{0x21,68,69,70,71}),10020);
  assert(f.events.size()==2&&f.events[0].id==0x7e9&&f.events[1].id==0x7e8);
  assert(f.events[0].data[9]==103&&f.events[1].data[9]==71);c.tick(11502);
  f.events.clear();assert(c.start(vin,2,a,12000));f.allow=false;
  c.receive(frame(0x7e8,{0x10,10,0x49,4,1,65,66,67}),12001);
  assert(f.saw("FLOW_CONTROL_TX_FAILED"));c.tick(13501);assert(!c.busy());
  puts("PASS: parser, addressing, matching, negative/pending, timeout, ISO-TP blocks/sequence/wrap, TX failure");
  static Fake pf; static obd::DiagnosticCore pc(pf); obd::Address pa;pa.tx=0x7e0;pa.rx=0x7e8;
  assert(pc.start(rpm,2,pa,100));pc.receive(frame(0x7e8,{4,0x41,0x0c,0,0}),105);pc.tick(105);assert(!pc.busy());
  assert(pc.start(rpm,2,pa,200));pc.receive(frame(0x7e8,{3,0x7f,1,0x78}),205);pc.tick(205);assert(pc.busy());pc.cancel();
  pc.setResponseWindow(200);assert(pc.start(rpm,2,pa,1000));pc.tick(1199);assert(pc.busy());pc.tick(1200);assert(!pc.busy());
  // Functional fast polling: complete every expected ECU, or retain collection timeout.
  static Fake fast;static obd::DiagnosticCore fc(fast);obd::Address functional;
  uint32_t ids[]={0x7e8,0x7e9};fc.setResponseWindow(70);
  assert(fc.start(rpm,2,functional,0,ids,1));
  fc.receive(frame(0x7e8,{4,0x41,0x0c,0,0}),8);fc.tick(8);assert(!fc.busy());
  assert(fc.start(rpm,2,functional,10,ids,2));
  fc.receive(frame(0x7e8,{4,0x41,0x0c,0,0}),18);fc.tick(18);assert(fc.busy());
  fc.receive(frame(0x7e9,{4,0x41,0x0c,0,0}),35);fc.tick(35);assert(!fc.busy());
  fast.events.clear();assert(fc.start(rpm,2,functional,100,ids,2));
  fc.receive(frame(0x7e8,{4,0x41,0x0c,0,0}),108);fc.tick(177);assert(fc.busy());
  fc.tick(178);assert(!fc.busy()&&fast.saw("EXPECTED_RESPONSE_MISSING"));
  fast.events.clear();assert(fc.start(rpm,2,functional,200));fc.tick(269);assert(fc.busy());
  fc.tick(270);assert(!fc.busy()&&fast.saw("NO_RESPONSE_TIMEOUT"));
  assert(fc.start(rpm,2,functional,300));
  fc.receive(frame(0x7e8,{4,0x41,0x0c,0,0}),369);fc.tick(370);assert(fc.busy());
  fc.tick(439);assert(!fc.busy()); // Unknown population: reload collection timer on response.
  assert(fc.start(rpm,2,functional,500,ids,1));
  fc.receive(frame(0x7e8,{3,0x7f,1,0x78}),508);fc.tick(600);assert(fc.busy());
  fc.receive(frame(0x7e8,{4,0x41,0x0c,0,0}),700);fc.tick(700);assert(!fc.busy());
  assert(fc.start(vin,2,functional,800,ids,1));
  fc.receive(frame(0x7e8,{0x10,10,0x49,4,1,65,66,67}),808);fc.tick(900);assert(fc.busy());
  fc.receive(frame(0x7e8,{0x21,68,69,70,71}),950);fc.tick(950);assert(!fc.busy());
  assert(fc.start(rpm,2,functional,1000,ids,1));
  fc.receive(frame(0x7e8,{3,0x7f,1,0x12}),1008);fc.tick(1008);assert(!fc.busy());
  assert(fc.start(rpm,2,functional,0xfffffff0u,ids,1));fc.tick(53);assert(fc.busy());fc.tick(54);assert(!fc.busy());
  puts("PASS: 70 ms timeout, expected responders, early completion, pending and multi-frame waits");

}
