#include "../TeensyOBDSDLogger/TraceFormat.h"
#include "../TeensyOBDSDLogger/LogReport.h"
#include <string>
#include <cassert>
#include <cstdio>
static std::string output;
void collect(const char *s){output+=s;assert(std::string(s).size()<320);}
int main(){
  char header[160];obd::formatElapsedHeader(header,sizeof header);
  assert(std::string(header)=="OBD-SkillLogger Version 1.0.0\n"
    "Time: elapsed milliseconds since power-on\n"
    "Calendar date: unavailable (no time sync)\n\n");
  // A rotated file declares the same power-on origin, not a new clock origin.
  char nextHeader[160];obd::formatElapsedHeader(nextHeader,sizeof nextHeader);
  assert(std::string(header)==nextHeader);
  uint8_t frame[]={2,1,0,0,0,0,0,0};
  obd::traceCan(collect,"TX",false,0x7df,124,frame,8);
  assert(output=="TX-CAN:  Std: 7DF   Time:    124   Len:  8   02 01 00 00 00 00 00 00\n");
  output.clear();obd::traceCan(collect,"RX",true,0x18daf110,131,frame,8,true);
  assert(output.find("Ext: 18DAF110")!=std::string::npos&&output.find("RTR")!=std::string::npos);
  uint8_t payload[4095];for(unsigned i=0;i<sizeof payload;++i)payload[i]=uint8_t(i);
  output.clear();obd::tracePayload(collect,"RX",0x7e8,134,payload,sizeof payload);
  std::string expected="    134  RX:  7E8   Len: 4095  ";char hex[4];
  for(auto byte:payload){snprintf(hex,sizeof hex," %02X",byte);expected+=hex;}expected+="\n";assert(output==expected);
  output.clear();uint8_t req[]={2,12,0};obd::traceRequest(collect,123,req,3);assert(output=="    123  Mode 2, PID 0C, Frame 0\n");
  // Screenshot-style request/response block, without background frames or status lines.
  output.clear();uint8_t request[]={1,0};uint8_t response[]={0x41,0,0xbe,0x3e,0xa8,0x13};
  uint8_t rawResponse[]={6,0x41,0,0xbe,0x3e,0xa8,0x13,0xaa};
  obd::traceCan(collect,"TX",false,0x7df,124,frame,8);
  obd::traceCan(collect,"RX",false,0x7e8,131,rawResponse,8);
  obd::traceRequest(collect,134,request,2);
  obd::tracePayload(collect,"TX",0x7df,125,request,2);
  obd::tracePayload(collect,"RX",0x7e8,134,response,6);
  assert(output=="TX-CAN:  Std: 7DF   Time:    124   Len:  8   02 01 00 00 00 00 00 00\n"
                 "RX-CAN:  Std: 7E8   Time:    131   Len:  8   06 41 00 BE 3E A8 13 AA\n"
                 "    134  Mode 1, PID 00\n"
                 "    125  TX:  7DF   Len:   2   01 00\n"
                 "    134  RX:  7E8   Len:   6   41 00 BE 3E A8 13\n");
  puts("PASS: CAN lines, remote/extended frames and full 4095-byte payload preservation");
}
