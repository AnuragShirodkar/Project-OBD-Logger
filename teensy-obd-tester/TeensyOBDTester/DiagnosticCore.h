#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace obd {
struct Frame { uint32_t id; bool extended; uint8_t len; uint8_t data[8]; };
struct Address {
  bool extended = false;
  uint32_t tx = 0x7DF, rx = 0; // rx=0: functional OBD response range
  bool accepts(const Frame &f) const {
    if (f.extended != extended) return false;
    if (rx) return f.id == rx;
    return extended ? (f.id & 0x1FFFFF00u) == 0x18DAF100u
                    : f.id >= 0x7E8 && f.id <= 0x7EF;
  }
  uint32_t flowId(uint32_t response) const {
    if (rx) return tx;
    return extended ? 0x18DA00F1u | ((response & 255u) << 8) : response - 8;
  }
};
inline bool parseHex(const char *s, uint8_t *bytes, size_t capacity, size_t &count) {
  count=0;
  while (*s) {
    while (*s==' ' || *s=='\t') ++s;
    if (!*s) break;
    unsigned v=0, digits=0;
    while (*s && *s!=' ' && *s!='\t') {
      unsigned d;
      if (*s>='0' && *s<='9') d=unsigned(*s-'0');
      else if (*s>='a' && *s<='f') d=unsigned(*s-'a'+10);
      else if (*s>='A' && *s<='F') d=unsigned(*s-'A'+10);
      else return false;
      if (++digits>2) return false;
      v=(v<<4)|d; ++s;
    }
    if (count==capacity) return false;
    bytes[count++]=uint8_t(v);
  }
  return count!=0;
}
class Sink {
public:
  virtual bool send(const Frame &) = 0;
  virtual void event(const char *, uint32_t, const uint8_t *, size_t) = 0;
  virtual ~Sink() {}
};
class DiagnosticCore {
  struct Rx {
    uint32_t id=0, last=0;
    uint16_t length=0, have=0;
    uint8_t next=1, block=0;
    bool used=false, receiving=false, pending=false;
    uint8_t bytes[4095];
  } states[8];
  Sink &sink;
  Address address;
  uint8_t request[7]={};
  size_t requestLen=0;
  uint32_t started=0;
  bool active=false, matched=false;
  void emit(const char *kind, uint32_t id=0) { sink.event(kind,id,nullptr,0); }
  Rx *state(uint32_t id, uint32_t now) {
    for (auto &s:states) if(s.used && s.id==id) return &s;
    for (auto &s:states) if(!s.used) {s.used=true;s.id=id;s.last=now;return &s;}
    emit("TOO_MANY_RESPONDERS",id);return nullptr;
  }
  bool matches(const uint8_t *p, size_t n) const {
    if(n>=3 && p[0]==0x7F && p[1]==request[0]) return true;
    if(!n || p[0]!=uint8_t(request[0]+0x40)) return false;
    if(requestLen>=2 && request[0]!=3 && request[0]!=4 && request[0]!=7 && request[0]!=10)
      return n>=2 && p[1]==request[1] &&
        (request[0]!=2 || requestLen<3 || (n>=3 && p[2]==request[2]));
    return true;
  }
  bool flow(Rx &s) {
    Frame f={}; f.id=address.flowId(s.id);f.extended=address.extended;
    f.len=8;f.data[0]=0x30;f.data[1]=8;f.data[2]=10; // BS=8, STmin=10 ms
    if(sink.send(f)) return true;
    s.receiving=false;s.pending=false;emit("FLOW_CONTROL_TX_FAILED",s.id);return false;
  }
  void complete(Rx &s,uint32_t now) {
    s.receiving=false;s.last=now;
    if(!matches(s.bytes,s.length)) {emit("UNMATCHED_PAYLOAD",s.id);return;}
    matched=true;
    if(s.bytes[0]==0x7F) {
      s.pending=s.bytes[2]==0x78;
      sink.event(s.pending?"RESPONSE_PENDING":"NEGATIVE",s.id,s.bytes,s.length);
    } else {
      s.pending=false;sink.event("POSITIVE",s.id,s.bytes,s.length);
    }
  }
public:
  explicit DiagnosticCore(Sink &s):sink(s) {}
  bool busy() const {return active;}
  void cancel() {if(active)emit("CANCELLED");active=false;}
  bool start(const uint8_t *p,size_t n,const Address &a,uint32_t now) {
    if(active || !p || !n || n>7 || p[0]<1 || p[0]>10)return false;
    for(auto &s:states) {
      s.used=s.receiving=s.pending=false;s.length=s.have=0;s.next=1;s.block=0;
    }
    address=a;memcpy(request,p,n);requestLen=n;started=now;matched=false;
    Frame f={};f.id=a.tx;f.extended=a.extended;f.len=8;f.data[0]=uint8_t(n);
    memcpy(f.data+1,p,n);
    if(!sink.send(f)){emit("REQUEST_TX_FAILED",a.tx);return false;}
    active=true;return true;
  }
  void receive(const Frame &f,uint32_t now) {
    if(!active || int32_t(now-started)<0 || !address.accepts(f) || !f.len || f.len>8)return;
    uint8_t pci=f.data[0]>>4;
    if(pci==0) {
      size_t len=f.data[0]&15;
      if(!len || len>7 || len+1>f.len){emit("BAD_SINGLE_FRAME",f.id);return;}
      if(!matches(f.data+1,len))return;
      Rx *s=state(f.id,now);if(!s)return;
      s->length=uint16_t(len);memcpy(s->bytes,f.data+1,len);complete(*s,now);
    } else if(pci==1) {
      if(f.len!=8){emit("BAD_FIRST_FRAME",f.id);return;}
      unsigned len=((f.data[0]&15u)<<8)|f.data[1];
      if(len<8 || len>4095){emit("BAD_ISOTP_LENGTH",f.id);return;}
      if(!matches(f.data+2,6))return;
      Rx *s=state(f.id,now);if(!s)return;
      s->length=uint16_t(len);s->have=6;s->next=1;s->block=0;
      s->receiving=true;s->pending=false;s->last=now;
      memcpy(s->bytes,f.data+2,6);matched=true;flow(*s);
    } else if(pci==2) {
      Rx *s=nullptr;
      for(auto &x:states)if(x.used && x.id==f.id && x.receiving){s=&x;break;}
      if(!s)return;
      if((f.data[0]&15)!=s->next){s->receiving=false;emit("ISOTP_SEQUENCE_ERROR",f.id);return;}
      size_t left=s->length-s->have,take=left<7?left:7;
      if(f.len<take+1){s->receiving=false;emit("SHORT_CONSECUTIVE_FRAME",f.id);return;}
      memcpy(s->bytes+s->have,f.data+1,take);s->have+=uint16_t(take);
      s->next=(s->next+1)&15;s->last=now;
      if(s->have==s->length)complete(*s,now);
      else if(++s->block==8){s->block=0;flow(*s);}
    }
  }
  void tick(uint32_t now) {
    if(!active)return;
    bool waiting=false;
    for(auto &s:states)if(s.used){
      if(s.receiving && uint32_t(now-s.last)>=1000){s.receiving=false;emit("ISOTP_TIMEOUT",s.id);}
      waiting|=s.receiving || s.pending;
    }
    if(uint32_t(now-started)>=10000){
      for(auto &s:states)if(s.used && (s.pending || s.receiving))emit("RESPONSE_DEADLINE",s.id);
      active=false;emit("REQUEST_END");
    } else if(uint32_t(now-started)>=1500 && !waiting){
      if(!matched)emit("NO_RESPONSE_TIMEOUT");
      active=false;emit("REQUEST_END");
    }
  }
};
} // namespace obd
