#pragma once
#include <ArduinoJson.h>
#include "DiagnosticCore.h"
#include "Schedule.h"
namespace automatic {
struct Config {
  uint32_t bitrate=500000,startup=5000,duration=0,responseTimeout=70;
  obd::Address address; Job jobs[64];size_t count=0;
};
inline bool hexId(const char *s,uint32_t &id){
  if(!s||!*s)return false;
  id=0;
  for(;*s;++s){unsigned d;
    if(*s>='0'&&*s<='9')d=*s-'0';else if(*s>='A'&&*s<='F')d=*s-'A'+10;
    else if(*s>='a'&&*s<='f')d=*s-'a'+10;else return false;
    if(id>(0x1FFFFFFFu-d)/16)return false;
    id=id*16+d;
  }return true;
}
inline bool readUInt(JsonObjectConst o,const char *key,uint32_t &value,uint32_t lo,uint32_t hi){
  if(o[key].isUnbound())return true;
  if(!o[key].is<uint32_t>())return false;
  value=o[key].as<uint32_t>();return value>=lo&&value<=hi;
}
inline bool parseConfig(JsonVariantConst input,Config &c,const char *&error){
  c=Config(); error="Invalid config.json";
  if(!input.is<JsonObjectConst>())return false;
  JsonObjectConst o=input.as<JsonObjectConst>();
  for(JsonPairConst kv:o){const char *k=kv.key().c_str();
    if(strcmp(k,"version")&&strcmp(k,"bitrate")&&strcmp(k,"address_bits")&&strcmp(k,"tx_id")&&strcmp(k,"rx_id")&&strcmp(k,"startup_delay_ms")&&strcmp(k,"run_duration_ms")&&strcmp(k,"response_timeout_ms")&&strcmp(k,"requests")){error="Unknown configuration key";return false;}
  }
  if(!o["version"].is<unsigned>()||o["version"].as<unsigned>()!=1){error="version must be 1";return false;}
  if(!readUInt(o,"bitrate",c.bitrate,125000,1000000)||
     (c.bitrate!=125000&&c.bitrate!=250000&&c.bitrate!=500000&&c.bitrate!=1000000)){error="Unsupported bitrate";return false;}
  uint32_t bits=11;
  if(!readUInt(o,"address_bits",bits,11,29)||(bits!=11&&bits!=29)){error="address_bits must be 11 or 29";return false;}
  c.address.extended=bits==29;c.address.tx=bits==29?0x18DB33F1:0x7DF;
  if(!o["tx_id"].isUnbound()&&(!o["tx_id"].is<const char*>()||!hexId(o["tx_id"],c.address.tx)))return false;
  if(!o["rx_id"].isUnbound()&&(!o["rx_id"].is<const char*>()||!hexId(o["rx_id"],c.address.rx)))return false;
  uint32_t limit=bits==29?0x1FFFFFFF:0x7FF, functional=bits==29?0x18DB33F1:0x7DF;
  if(c.address.tx>limit||c.address.rx>limit||(!c.address.rx&&c.address.tx!=functional)||(c.address.rx&&c.address.tx==functional)||c.address.rx==c.address.tx){error="Invalid functional/physical address pair";return false;}
  if(!readUInt(o,"startup_delay_ms",c.startup,0,600000)||!readUInt(o,"run_duration_ms",c.duration,0,86400000)||!readUInt(o,"response_timeout_ms",c.responseTimeout,10,10000)){error="Timing outside allowed range";return false;}
  if(c.duration&&c.duration<1000){error="Nonzero run duration must be at least 1000 ms";return false;}
  if(!o["requests"].is<JsonArrayConst>()){error="requests must be an array";return false;}
  JsonArrayConst a=o["requests"].as<JsonArrayConst>();
  if(a.size()<1||a.size()>64){error="Use 1 to 64 requests";return false;}
  for(JsonVariantConst v:a){
    if(!v.is<JsonObjectConst>())return false;
    JsonObjectConst item=v.as<JsonObjectConst>();
    for(JsonPairConst kv:item)if(strcmp(kv.key().c_str(),"request")&&strcmp(kv.key().c_str(),"interval_ms"))return false;
    Job &j=c.jobs[c.count];
    if(!item["request"].is<const char*>()||!obd::parseHex(item["request"],j.bytes,3,j.length)){error="Invalid request hex bytes";return false;}
    unsigned mode=j.bytes[0];
    if(!((mode==1&&j.length==2)||(mode==2&&j.length==3)||((mode==3||mode==7||mode==10)&&j.length==1)||((mode==6||mode==9)&&j.length==2))){error="Only modes 01,02,03,06,07,09,0A with correct parameters allowed";return false;}
    if(item["interval_ms"].isUnbound()||!readUInt(item,"interval_ms",j.interval,10,3600000)){error="interval_ms must be 10..3600000";return false;}
    for(size_t i=0;i<c.count;++i)if(c.jobs[i].length==j.length&&!memcmp(c.jobs[i].bytes,j.bytes,j.length)){error="Duplicate request";return false;}
    ++c.count;
  }
  error=nullptr;return true;
}
}
