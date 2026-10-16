#include "../TeensyOBDSDLogger/Config.h"
#include <assert.h>
#include <fstream>
#include <iostream>
bool parse(const char *text){JsonDocument d;assert(!deserializeJson(d,text));automatic::Config c;const char *error;return automatic::parseConfig(d.as<JsonVariantConst>(),c,error);}
int main(){
  JsonDocument d;std::ifstream file("config.json");assert(!deserializeJson(d,file));
  automatic::Config c;const char *error;assert(automatic::parseConfig(d.as<JsonVariantConst>(),c,error));
  assert(c.duration==0&&c.bitrate==500000&&c.count==6&&c.jobs[5].interval==1000&&c.jobs[1].bytes[2]==0);
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
