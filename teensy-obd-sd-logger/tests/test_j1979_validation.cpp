#include "../TeensyOBDSDLogger/DiagnosticCore.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using obd::DataValidation;
struct ValidationSink:obd::Sink {
  std::vector<std::string> events;std::vector<uint8_t> invalid;
  bool send(const obd::Frame &)override{return true;}
  void event(const char *k,uint32_t,const uint8_t *p,size_t n)override {
    events.push_back(k);
    if(!strcmp(k,"INVALID_LENGTH")||!strcmp(k,"INVALID_VALUE"))invalid.assign(p,p+n);
  }
  bool saw(const char *k)const {for(auto &e:events)if(e==k)return true;return false;}
};
int main() {
  using namespace obd;
  assert(validateData(nullptr,0)==DataValidation::WrongLength);
  uint8_t p[32]={0x41,0x01,0x81,0x07,0,0};
  assert(validateData(p,6)==DataValidation::LayoutChecked);
  assert(validateData(p,5)==DataValidation::WrongLength);
  assert(validateData(p,7)==DataValidation::WrongLength);
  // No invented length for identifiers not yet reviewed, even if they are real PIDs.
  p[1]=0x66;assert(validateData(p,7)==DataValidation::Unverified);
  // Every reviewed fixed-length PID: short/exact/long, live and freeze-frame.
  for(unsigned pid=0;pid<256;++pid) {
    size_t length=reviewedPidLength(uint8_t(pid));if(!length)continue;
    for(unsigned service=0x41;service<=0x42;++service) {
      memset(p,0,sizeof p);p[0]=uint8_t(service);p[1]=uint8_t(pid);
      size_t n=length+(service==0x42?3:2);
      assert(validateData(p,n)==DataValidation::LayoutChecked);
      assert(validateData(p,n-1)==DataValidation::WrongLength);
      assert(validateData(p,n+1)==DataValidation::WrongLength);
    }
  }
  for(uint8_t service: {0x41,0x42,0x46,0x49}) {
    memset(p,0,sizeof p);p[0]=service;p[1]=0xE0;size_t n=service==0x42?7:6;
    p[n-1]=2;assert(validateData(p,n)==DataValidation::LayoutChecked);
    p[n-1]=3;assert(validateData(p,n)==DataValidation::InvalidValue);
  }
  uint8_t vin[21]={0x49,2,1};memcpy(vin+3,"123456789ABCDEFGH",17);
  assert(validateData(vin,20)==DataValidation::LayoutChecked);
  for(uint8_t bad: {0,32,73,79,81,97,255}) {
    vin[3]=bad;assert(validateData(vin,20)==DataValidation::InvalidValue);
  }
  vin[3]='1';vin[2]=2;assert(validateData(vin,20)==DataValidation::InvalidValue);
  vin[2]=1;assert(validateData(vin,19)==DataValidation::WrongLength);
  assert(validateData(vin,21)==DataValidation::WrongLength);
  uint8_t negative[]={0x7F,1,0x78,0};
  assert(validateData(negative,3)==DataValidation::LayoutChecked);
  assert(validateData(negative,4)==DataValidation::WrongLength);
  uint8_t clear[]={0x44,0};assert(validateData(clear,1)==DataValidation::LayoutChecked);
  assert(validateData(clear,2)==DataValidation::WrongLength);
  // Invalid matching response must not complete a known physical ECU or become POSITIVE.
  static ValidationSink sink;static DiagnosticCore core(sink);Address a;a.tx=0x7E0;a.rx=0x7E8;
  uint8_t throttle[]={1,0x11};assert(core.start(throttle,2,a,0));
  Frame f={};f.id=a.rx;f.len=8;f.data[0]=4;f.data[1]=0x41;f.data[2]=0x11;f.data[3]=128;f.data[4]=0;
  core.receive(f,1);core.tick(1);
  assert(core.busy()&&sink.saw("INVALID_LENGTH")&&!sink.saw("POSITIVE"));
  assert(sink.invalid==std::vector<uint8_t>({0x41,0x11,128,0}));
  // Correct payload with padding outside ISO-TP length is valid and can complete.
  f.data[0]=3;core.receive(f,2);core.tick(2);assert(!core.busy()&&sink.saw("POSITIVE"));
  // Unknown data stays observable and explicitly unverified, never falsely rejected.
  sink.events.clear();uint8_t unknown[]={6,0x21};assert(core.start(unknown,2,a,10));
  f.data[0]=2;f.data[1]=0x46;f.data[2]=0x21;core.receive(f,11);core.tick(11);
  assert(sink.saw("UNVERIFIED_DEFINITION")&&sink.saw("POSITIVE"));
  puts("PASS: reviewed fixed lengths, reserved bitmap bit, VIN characters, raw invalid retention, unknown definitions and completion gating");
}
