extern "C" {
#include "obd_sim.h"
}
#include "../../teensy-obd-sd-logger/TeensyOBDSDLogger/DiagnosticCore.h"
#include "../../teensy-obd-sd-logger/TeensyOBDSDLogger/Discovery.h"
#include "../../teensy-obd-sd-logger/TeensyOBDSDLogger/Config.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
struct TestSink:obd::Sink {
  automatic::Discovery discovery;
  ObdFrame replies[4];size_t count=0,index=0;uint32_t now=0;
  std::vector<uint32_t> responders;
  bool send(const obd::Frame &f)override{
    ObdFrame q={};q.id=f.id;q.dlc=f.len;for(unsigned i=0;i<f.len;++i)q.data[i]=f.data[i];
    count=obd_reply(&q,now,replies);index=0;return true;
  }
  void event(const char *kind,uint32_t id,const uint8_t *data,size_t n)override{
    if(!strcmp(kind,"POSITIVE")){responders.push_back(id);if(discovery.busy())assert(discovery.receive(id,data,n));}
    assert(strcmp(kind,"NO_RESPONSE_TIMEOUT")&&strcmp(kind,"EXPECTED_RESPONSE_MISSING"));
  }
};
int main(){
  ObdFrame q={};q.id=0x7df;q.dlc=8;q.data[0]=2;q.data[1]=1;ObdFrame r[4];
  assert(obd_reply(&q,100,r)==4);
  const uint8_t maps[4][4]={{8,16,0,0},{0,24,0,0},{8,0,0,0},{0,8,0,0}};
  for(unsigned i=0;i<4;++i){assert(r[i].id==0x7e8+i&&r[i].due==102+i*2);assert(r[i].data[0]==6&&!memcmp(r[i].data+3,maps[i],4));}
  q.data[2]=12;assert(obd_reply(&q,0,r)==2);assert(r[0].data[3]==15&&r[0].data[4]==160&&r[1].data[3]==31&&r[1].data[4]==64);
  q.data[2]=5;assert(obd_reply(&q,0,r)==2&&r[0].data[3]==120&&r[1].id==0x7ea&&r[1].data[3]==130);
  q.data[2]=13;assert(obd_reply(&q,0,r)==2&&r[0].data[3]==25&&r[1].data[3]==50);
  q.data[2]=47;assert(obd_reply(&q,0,r)==0);
  q.id=0x7e2;assert(obd_reply(&q,0,r)==1&&r[0].data[1]==127);
  q.id=0x7e1;q.data[2]=12;assert(obd_reply(&q,0,r)==1&&r[0].id==0x7e9);
  q.data[0]=1;q.data[1]=4;assert(obd_reply(&q,0,r)==0); // No clear service in Stage 1.
  q.data[0]=0x10;assert(obd_reply(&q,0,r)==0);
  q.data[0]=2;q.data[1]=1;q.dlc=2;assert(obd_reply(&q,0,r)==0);
  q.dlc=8;q.id=0x123;assert(obd_reply(&q,0,r)==0);
  q.id=0x7df;q.data[2]=0;assert(obd_reply(&q,0xfffffffeu,r)==4&&r[0].due==0);
  TestSink sink;obd::DiagnosticCore core(sink);core.setResponseWindow(70);obd::Address address;
  auto exchange=[&](const uint8_t *bytes,size_t n){
    sink.responders.clear();uint32_t expected[8];size_t count=sink.discovery.expected(bytes,n,expected);
    assert(core.start(bytes,n,address,sink.now,expected,count));
    unsigned elapsed=0;
    while(core.busy()&&elapsed++<200){
      while(sink.index<sink.count&&(int32_t)(sink.now-sink.replies[sink.index].due)>=0){
        const auto &r=sink.replies[sink.index++];obd::Frame f={};f.id=r.id;f.len=r.dlc;memcpy(f.data,r.data,8);core.receive(f,sink.now);
      }
      core.tick(sink.now);++sink.now;
    }assert(!core.busy());
  };
  sink.discovery.begin();
  while(sink.discovery.busy()){uint8_t bytes[3];size_t n=sink.discovery.request(bytes);exchange(bytes,n);assert(sink.responders.size()==4);sink.discovery.finishTransaction();}
  assert(sink.discovery.count==4);
  JsonDocument doc;std::ifstream file("teensy-stage1-config.json");assert(!deserializeJson(doc,file));
  automatic::Config config;const char *error;assert(automatic::parseConfig(doc.as<JsonVariantConst>(),config,error));
  unsigned enabled=0;for(size_t i=0;i<config.count;++i){auto &job=config.jobs[i];
    job.enabled=sink.discovery.requestSupport(job.bytes,job.length)!=automatic::Discovery::Unsupported;
    if(job.enabled){++enabled;exchange(job.bytes,job.length);assert(sink.responders.size()==(job.bytes[1]==0?4:2));}
    else assert(job.bytes[1]==47);
  }assert(enabled==4&&config.count==5);
  puts("PASS: exact ECU maps/values, physical addressing, malformed requests, wrap, real Teensy discovery/core/config integration (4 ECUs, 4 enabled, 1 skipped)");
}
