#include "../TeensyOBDSDLogger/Config.h"
#include <assert.h>
#include <fstream>
#include <iostream>
bool parse(const char *text){JsonDocument d;assert(!deserializeJson(d,text));automatic::Config c;const char *error;return automatic::parseConfig(d.as<JsonVariantConst>(),c,error);}
int main(){
  JsonDocument d;std::ifstream file("config.json");assert(!deserializeJson(d,file));
  automatic::Config c;const char *error;assert(automatic::parseConfig(d.as<JsonVariantConst>(),c,error));
  assert(c.duration==0&&c.bitrate==500000&&c.count==51&&c.responseTimeout==70);
  bool mode6=false;for(size_t i=0;i<c.count;++i){assert(c.jobs[i].bytes[0]!=4&&c.jobs[i].bytes[0]!=5&&c.jobs[i].bytes[0]!=8);if(c.jobs[i].bytes[0]==6)mode6=true;}assert(mode6);
  assert(parse(R"({"version":1,"requests":[{"request":"06 00","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"requests":[{"request":"06","interval_ms":10}]})"));
  for(const char *blocked:{"05 00","08 00"}){JsonDocument bad=d;bad["requests"][0]["request"]=blocked;automatic::Config ignored;assert(!automatic::parseConfig(bad.as<JsonVariantConst>(),ignored,error));}
  JsonDocument capacity;capacity["version"]=1;auto entries=capacity["requests"].to<JsonArray>();
  for(unsigned i=0;i<64;++i){char request[8];snprintf(request,sizeof request,"01 %02X",i);auto item=entries.add<JsonObject>();item["request"]=request;item["interval_ms"]=10;}
  automatic::Config large;assert(automatic::parseConfig(capacity.as<JsonVariantConst>(),large,error)&&large.count==64);
  auto extra=entries.add<JsonObject>();extra["request"]="01 40";extra["interval_ms"]=10;
  assert(!automatic::parseConfig(capacity.as<JsonVariantConst>(),large,error));
  // Keep scheduler timing tests independent of the shipped request list.
  JsonDocument fixture;assert(!deserializeJson(fixture,R"({"version":1,"requests":[{"request":"01 0C","interval_ms":10},{"request":"02 0C 00","interval_ms":10},{"request":"03","interval_ms":10},{"request":"07","interval_ms":10},{"request":"0A","interval_ms":10},{"request":"09 02","interval_ms":1000}]})"));
  assert(automatic::parseConfig(fixture.as<JsonVariantConst>(),c,error));
  assert(parse(R"({"version":1,"requests":[{"request":"03","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"requests":[{"request":"04","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"requests":[{"request":"02 0C","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"requests":[{"request":"03","interval_ms":1}]})"));
  assert(!parse(R"({"version":1,"bitrate":333333,"requests":[{"request":"03","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"bitrate":-1,"requests":[{"request":"03","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"bitrte":500000,"requests":[{"request":"03","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"tx_id":"7DF","rx_id":"7E8","requests":[{"request":"03","interval_ms":10}]})"));
  assert(parse(R"({"version":1,"tx_id":"7E0","rx_id":"7E8","requests":[{"request":"03","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"requests":[{"request":"03","interval_ms":10},{"request":"03","interval_ms":20}]})"));
  assert(parse(R"({"version":1,"run_duration_ms":0,"requests":[{"request":"03","interval_ms":10}]})"));
  assert(!parse(R"({"version":1,"run_duration_ms":1,"requests":[{"request":"03","interval_ms":10}]})"));
  assert(parse(R"({"version":1,"run_duration_ms":60000,"requests":[{"request":"03","interval_ms":10}]})"));
  automatic::Schedule s;s.begin(c.jobs,c.count,100);
  for(int i=0;i<6;++i)assert(s.select(c.jobs,c.count,100)==i);
  assert(s.select(c.jobs,c.count,100)==-1);
  assert(s.select(c.jobs,c.count,180)==0);assert(c.jobs[0].skipped==7&&c.jobs[0].next==190);
  for(int i=1;i<5;++i)assert(s.select(c.jobs,c.count,180)==i);
  assert(s.select(c.jobs,c.count,180)==-1);
  s.begin(c.jobs,c.count,0xfffffffau);assert(s.select(c.jobs,c.count,0xfffffffau)==0);
  assert(c.jobs[0].next==4);s.finish(c.jobs,c.count,14);assert(c.jobs[0].next==24);
  // Fairness: even when all 10 ms jobs are overdue, the one-second job gets a turn.
  s.begin(c.jobs,c.count,0);for(int i=0;i<12;++i)assert(s.select(c.jobs,c.count,2000+i*1500)==i%6);
  std::cout<<"Config and scheduling tests passed\n";
}
