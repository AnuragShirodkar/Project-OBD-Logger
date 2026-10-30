#include "../TeensyOBDSDLogger/Config.h"
#include <fstream>
#include <string>
#include <iterator>
#include <cstdio>
// This is a coverage report, NOT a standards-conformance test or score.
int main(int argc,char **argv) {
  const char *path=argc>1?argv[1]:"config.json";
  std::ifstream input(path,std::ios::binary);
  if(!input){fprintf(stderr,"Cannot read configuration: %s\n",path);return 1;}
  std::string text((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
  JsonDocument doc;auto jsonError=deserializeJson(doc,text);
  if(jsonError){fprintf(stderr,"Invalid JSON: %s\n",jsonError.c_str());return 1;}
  automatic::Config config;const char *error=nullptr;
  if(!automatic::parseConfig(doc.as<JsonVariantConst>(),config,error)) {
    fprintf(stderr,"Invalid logger configuration: %s\n",error?error:"unknown error");return 1;
  }
  unsigned checked=0;
  for(size_t i=0;i<config.count;++i) {
    const auto &job=config.jobs[i];
    bool rule=job.length>=2&&obd::hasReviewedLayout(uint8_t(job.bytes[0]+0x40),job.bytes[1]);
    if(rule){++checked;continue;}
    printf("No installed layout rule:");
    for(size_t j=0;j<job.length;++j)printf(" %02X",job.bytes[j]);
    puts("");
  }
  printf("Configured-job layout-rule coverage: %u/%u (%.1f%%)\n",checked,
         unsigned(config.count),100.0*checked/config.count);
  puts("Not overall J1979-DA compliance. Does not measure semantics, unconfigured identifiers, transport or certification.");
}
