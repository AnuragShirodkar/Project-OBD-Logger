#include "../TeensyOBDSDLogger/TraceFormat.h"
#include <string>
#include <cassert>
#include <cstdio>
static std::string output;
void collect(const char *s){output+=s;assert(std::string(s).size()<320);}
int main(){
  uint8_t frame[]={2,1,0,0,0,0,0,0};
  obd::traceCan(collect,"TX",false,0x7df,124,frame,8);
  assert(output=="TX-CAN:  Std: 7DF   Time:        124   Len:  8   02 01 00 00 00 00 00 00\n");
  output.clear();obd::traceCan(collect,"RX",true,0x18daf110,131,frame,8,true);
  assert(output.find("Ext: 18DAF110")!=std::string::npos&&output.find("RTR")!=std::string::npos);
  uint8_t payload[4095];for(unsigned i=0;i<sizeof payload;++i)payload[i]=uint8_t(i);
  output.clear();obd::tracePayload(collect,"RX",0x7e8,134,payload,sizeof payload);
  std::string expected="       134  RX:  7E8   Len: 4095  ";char hex[4];
  for(auto byte:payload){snprintf(hex,sizeof hex," %02X",byte);expected+=hex;}expected+="\n";assert(output==expected);
  output.clear();uint8_t req[]={2,12,0};obd::traceRequest(collect,123,req,3);assert(output=="       123  Mode 2, PID 0C, Frame 0\n");
  puts("PASS: Silver-style CAN lines, remote/extended frames and full 4095-byte payload preservation");
}
