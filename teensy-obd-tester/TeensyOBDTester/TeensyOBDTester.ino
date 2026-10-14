#include <FlexCAN_T4.h>
#include <SD.h>
#include "DiagnosticCore.h"

// CAN1 default pins: TX=22, RX=23. TLE9371VSJ STB is grounded externally.
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can;
File logfile;
obd::Address addressing;
uint32_t bitrate=0, configuredRate=500000, transaction=0;
bool ready=false, scanning=false, restartBus=false, sdGood=false;
const uint32_t scanRates[]={500000,250000,125000,1000000};
unsigned scanIndex=0, scanFrames=0;
uint32_t scanStart=0, flushAt=0;

// ISR only copies frames. Never perform SD or USB work inside the interrupt.
struct Captured { CAN_message_t frame; uint32_t ms; };
Captured captures[512];
volatile uint16_t captureHead=0,captureTail=0;
volatile uint32_t captureDrops=0;
uint32_t reportedCaptureDrops=0;
struct LogLine {char text[160];};
LogLine logQueue[512];
unsigned logHead=0,logTail=0,logCount=0;
uint32_t logDrops=0,consoleDrops=0,reportedLogDrops=0;
char consoleQueue[32768];
unsigned consoleHead=0,consoleTail=0,consoleCount=0;
bool loggingFault=false;
char logName[24]={};

void queueConsole(const char *s) {
  size_t n=strlen(s);
  if(!Serial)return;
  if(n>sizeof(consoleQueue)-consoleCount){++consoleDrops;return;}
  for(size_t i=0;i<n;++i){
    consoleQueue[consoleHead]=s[i];consoleHead=(consoleHead+1)%sizeof(consoleQueue);++consoleCount;
  }
}
void pumpConsole(){
  if(!Serial){consoleHead=consoleTail=consoleCount=0;return;}
  unsigned available=Serial.availableForWrite();
  unsigned n=consoleCount;
  if(n>available)n=available;
  if(n>sizeof(consoleQueue)-consoleTail)n=sizeof(consoleQueue)-consoleTail;
  if(n){size_t wrote=Serial.write((const uint8_t*)consoleQueue+consoleTail,n);
    consoleTail=(consoleTail+wrote)%sizeof(consoleQueue);consoleCount-=wrote;}
}
void recordAt(uint32_t when,const char *kind,uint32_t id,const char *text){
  if(logCount==512){++logDrops;loggingFault=true;return;}
  snprintf(logQueue[logHead].text,160,"%lu,%lu,%lu,%s,%08lX,\"%s\"\n",
    (unsigned long)when,(unsigned long)transaction,(unsigned long)configuredRate,
    kind,(unsigned long)id,text?text:"");
  logHead=(logHead+1)%512;++logCount;
}
void note(const char *kind,const char *text){recordAt(millis(),kind,0,text);}
void raw(const char *kind,const obd::Frame &f,uint32_t when){
  char b[72];int pos=snprintf(b,sizeof b,"%s DLC=%u",f.extended?"EXT":"STD",f.len);
  for(unsigned i=0;i<f.len&&i<8;++i)pos+=snprintf(b+pos,sizeof(b)-pos," %02X",f.data[i]);
  recordAt(when,kind,f.id,b);
}
void pumpLog(){
  if(!logCount)return;
  const char *s=logQueue[logTail].text;size_t n=strlen(s);
  if(sdGood && logfile.write((const uint8_t*)s,n)!=n){
    sdGood=false;loggingFault=true;queueConsole("SD_WRITE_FAILED; request stopped; restart after checking SD\n");
  }
  queueConsole(s);logTail=(logTail+1)%512;--logCount;
}
void onCan(const CAN_message_t &f){
  unsigned next=(captureHead+1)%512;
  if(next==captureTail){++captureDrops;return;}
  captures[captureHead].frame=f;captures[captureHead].ms=millis();asm volatile("dmb" ::: "memory");captureHead=next;
}
bool popCapture(Captured &c){
  noInterrupts();
  if(captureHead==captureTail){interrupts();return false;}
  c=captures[captureTail];captureTail=(captureTail+1)%512;
  interrupts();return true;
}
void configureCan(uint32_t rate,bool listen){
  ready=false;
  can.begin();can.setMaxMB(16);can.setBaudRate(rate,LISTEN_ONLY);
  can.enableFIFO();can.setFIFOFilter(ACCEPT_ALL);
  can.onReceive(onCan);
  noInterrupts();captureHead=captureTail=0;interrupts();
  can.enableFIFOInterrupt();
  if(!listen)can.setBaudRate(rate,TX);
  configuredRate=rate;ready=!listen;
}
const char *nrcName(uint8_t code){
  switch(code){
    case 0x10:return "generalReject";case 0x11:return "serviceNotSupported";
    case 0x12:return "subFunctionNotSupported";case 0x13:return "incorrectLengthOrFormat";
    case 0x21:return "busyRepeatRequest";case 0x22:return "conditionsNotCorrect";
    case 0x31:return "requestOutOfRange";case 0x33:return "securityAccessDenied";
    case 0x78:return "responsePending";default:return "unmapped NRC; retain raw value";
  }
}
class HardwareSink:public obd::Sink{
public:
  bool send(const obd::Frame &f)override{
    if(!ready || !sdGood || loggingFault)return false;
    CAN_message_t msg;msg.id=f.id;msg.flags.extended=f.extended;msg.len=f.len;
    memcpy(msg.buf,f.data,8);msg.seq=false;
    // Check and write an inactive mailbox atomically: never enter library TX queue.
    int accepted=0;
    noInterrupts();
    for(unsigned i=8;i<16;++i){
      if(FLEXCAN_get_code(FLEXCANb_MBn_CS(CAN1,i))==FLEXCAN_MB_CODE_TX_INACTIVE){
        accepted=can.write((FLEXCAN_MAILBOX)i,msg);break;
      }
    }
    interrupts();
    if(accepted==1){raw(f.data[0]==0x30?"FLOW_TX_SUBMIT":"TX_SUBMIT",f,millis());return true;}
    note("TX_FAILED","No free hardware mailbox");return false;
  }
  void event(const char *kind,uint32_t id,const uint8_t *p,size_t n)override{
    if(!n)recordAt(millis(),kind,id,"");
    for(size_t offset=0;offset<n;offset+=24){
      char b[110];int pos=snprintf(b,sizeof b,"len=%u offset=%u",(unsigned)n,(unsigned)offset);
      for(size_t i=offset;i<n&&i<offset+24;++i)pos+=snprintf(b+pos,sizeof(b)-pos," %02X",p[i]);
      recordAt(millis(),kind,id,b);
    }
    if(n>=3 && p[0]==0x7F)recordAt(millis(),"NRC",id,nrcName(p[2]));
    if(n>=3 && p[0]==0x41){
      char b[72]={};
      if(p[1]==0x0C&&n>=4)snprintf(b,sizeof b,"RPM=%.2f",((p[2]<<8)|p[3])/4.0);
      if(p[1]==0x0D)snprintf(b,sizeof b,"Speed=%u km/h",p[2]);
      if(p[1]==0x05)snprintf(b,sizeof b,"Coolant=%d C",int(p[2])-40);
      if(b[0])recordAt(millis(),"DECODED",id,b);
    }
    if(!strcmp(kind,"REQUEST_END") || !strcmp(kind,"CANCELLED"))restartBus=true;
  }
} sink;
obd::DiagnosticCore diagnostics(sink);
void startScan(){
  diagnostics.cancel();restartBus=false;bitrate=0;scanning=true;scanIndex=0;scanFrames=0;
  configureCan(scanRates[scanIndex],true);scanStart=millis();
  note("AUTO_LISTEN","500000; no diagnostic requests sent");
}
void help(){
  note("HELP","auto | baud 500000 | addr 11 | addr 29 | target TX RX | functional");
  note("HELP","01 00 | 01 0C | 01 05 | 03 | 09 02 | status | stop | close | help");
  note("HELP","Hex bytes; newline required. 04 clears DTCs; 08 requests control actions.");
  note("HELP","Use full request bytes. Mode-only 01 becomes 01 00; 09 becomes 09 00.");
}
void status(){
  char b[110];snprintf(b,sizeof b,"rate=%lu ready=%u scan=%u busy=%u sd=%u file=%s",
    (unsigned long)bitrate,ready,scanning,diagnostics.busy(),sdGood,logName);
  note("STATUS",b);
  snprintf(b,sizeof b,"TX=%08lX RX=%08lX ext=%u RXdrops=%lu logDrops=%lu consoleDrops=%lu",
    (unsigned long)addressing.tx,(unsigned long)addressing.rx,addressing.extended,
    (unsigned long)captureDrops,(unsigned long)logDrops,(unsigned long)consoleDrops);
  note("STATUS",b);
}
bool number(const char *s,unsigned base,uint32_t &out){
  if(!*s)return false;
  uint32_t v=0;
  for(;*s;++s){
    unsigned d;
    if(*s>='0'&&*s<='9')d=*s-'0';
    else if(base==16&&*s>='a'&&*s<='f')d=*s-'a'+10;
    else if(base==16&&*s>='A'&&*s<='F')d=*s-'A'+10;
    else return false;
    if(d>=base || v>(UINT32_MAX-d)/base)return false;
    v=v*base+d;
  }
  out=v;return true;
}
void command(char *s){
  size_t len=strlen(s);while(len&&(s[len-1]==' '||s[len-1]=='\t'))s[--len]=0;
  while(*s==' '||*s=='\t')++s;
  if(!*s)return;
  if(!strcmp(s,"help")){help();return;}
  if(!strcmp(s,"status")){status();return;}
  if(!strcmp(s,"stop")||!strcmp(s,"close")){
    diagnostics.cancel();scanning=false;ready=false;bitrate=0;restartBus=false;
    configureCan(configuredRate,true);note("STOP","Listen-only; no pending requests");
    if(!strcmp(s,"close")){
      while(logCount){pumpLog();pumpConsole();}
      logfile.flush();logfile.close();sdGood=false;
      queueConsole("LOG_CLOSED; SD can be removed; reset to open a new log\n");
    }
    return;
  }
  if(diagnostics.busy()){note("BUSY","Wait for REQUEST_END or type stop");return;}
  if(!strcmp(s,"auto")){startScan();return;}
  if(!strncmp(s,"baud ",5)){
    uint32_t rate=0;
    if(!number(s+5,10,rate)||(rate!=125000&&rate!=250000&&rate!=500000&&rate!=1000000)){
      note("INPUT_ERROR","Use baud 125000 / 250000 / 500000 / 1000000");return;}
    scanning=false;restartBus=false;configureCan(rate,false);bitrate=rate;
    note("BAUD_MANUAL","Selected by user; not automatically verified");return;
  }
  if(!strcmp(s,"addr 11")||!strcmp(s,"addr 29")){
    addressing.extended=!strcmp(s,"addr 29");
    addressing.tx=addressing.extended?0x18DB33F1:0x7DF;addressing.rx=0;
    status();return;
  }
  if(!strcmp(s,"functional")){
    addressing.tx=addressing.extended?0x18DB33F1:0x7DF;addressing.rx=0;status();return;
  }
  if(!strncmp(s,"target ",7)){
    char *split=strchr(s+7,' ');uint32_t tx,rx;
    if(!split){note("INPUT_ERROR","target TX RX; identifiers in hex");return;}
    *split=0;
    if(!number(s+7,16,tx)||!number(split+1,16,rx)||!rx||
       tx>(addressing.extended?0x1FFFFFFFu:0x7FFu)||rx>(addressing.extended?0x1FFFFFFFu:0x7FFu)){
      note("INPUT_ERROR","Invalid target IDs");return;}
    // Functional IDs cannot serve as a physical FC target.
    if(tx==0x7DF||tx==0x18DB33F1){note("INPUT_ERROR","Use functional command for broadcast addressing");return;}
    addressing.tx=tx;addressing.rx=rx;status();return;
  }
  if(!ready||scanning){note("NOT_READY","Run auto or select baud manually; a silent ECU cannot reveal its rate");return;}
  if(!sdGood||loggingFault){queueConsole("LOG_NOT_READY; check SD and reset before sending\n");return;}
  uint8_t p[7];size_t n=0;
  if(!obd::parseHex(s,p,sizeof p,n)||p[0]<1||p[0]>10){
    note("INPUT_ERROR","Enter mode 01..0A followed by hex parameter bytes");return;
  }
  if(n==1&&(p[0]==1||p[0]==9)){p[n++]=0;}
  if((p[0]==1||p[0]==9||p[0]==6)&&n!=2){
    note("INPUT_ERROR","This tester requires mode plus one PID/MID");return;}
  if(p[0]==2&&n!=3){note("INPUT_ERROR","Mode 02 requires PID and freeze-frame number");return;}
  if((p[0]==3||p[0]==4||p[0]==7||p[0]==10)&&n!=1){
    note("INPUT_ERROR","This mode takes no parameter bytes");return;}
  if((p[0]==5||p[0]==8)&&n<2){note("INPUT_ERROR","Supply the service-specific identifier/data");return;}
  ++transaction;
  if(!diagnostics.start(p,n,addressing,millis()))note("START_FAILED","Request not started");
}
char input[100];unsigned inputLength=0;bool discardLine=false;
void serialInput(){
  for(unsigned i=0;i<128&&Serial.available();++i){
    char c=Serial.read();
    if(c=='\r'||c=='\n'){
      if(discardLine)note("INPUT_ERROR","Line too long; nothing sent");
      else if(inputLength){input[inputLength]=0;command(input);}
      inputLength=0;discardLine=false;
    }else if(inputLength<sizeof(input)-1&&!discardLine)input[inputLength++]=c;
    else discardLine=true;
  }
}
void setup(){
  Serial.begin(115200);
  uint32_t wait=millis();while(!Serial&&millis()-wait<2500){}
  if(SD.begin(BUILTIN_SDCARD)){
    for(unsigned i=0;i<10000;++i){
      snprintf(logName,sizeof logName,"OBD%04u.CSV",i);
      if(!SD.exists(logName)){logfile=SD.open(logName,FILE_WRITE);break;}
    }
    sdGood=bool(logfile);
  }
  if(sdGood){
    const char *header="ms,request,bitrate,event,can_id,detail\n";
    sdGood=logfile.write((const uint8_t*)header,strlen(header))==strlen(header);
  }
  if(!sdGood)queueConsole("SD_INIT_FAILED; requests disabled; use FAT32 SD in Teensy's built-in slot\n");
  note("BOOT","Teensy 4.1 CAN1 TX22 RX23; STB grounded; timestamps=uptime milliseconds");
  help();startScan();
}
void loop(){
  Captured c;
  for(unsigned i=0;i<64&&popCapture(c);++i){
    obd::Frame f={};f.id=c.frame.id;f.extended=c.frame.flags.extended;f.len=c.frame.len;
    memcpy(f.data,c.frame.buf,8);
    raw(c.frame.flags.remote?"RX_REMOTE":"RX",f,c.ms);
    if(c.frame.flags.overrun){note("CAN_OVERRUN","Hardware reported receive overrun");loggingFault=true;}
    if(scanning){
      if(!c.frame.flags.remote&&f.len<=8)++scanFrames;
    }else if(!c.frame.flags.remote)diagnostics.receive(f,c.ms);
  }
  if(captureDrops!=reportedCaptureDrops){
    reportedCaptureDrops=captureDrops;note("RX_QUEUE_OVERFLOW","Capture lost; requests disabled until reset");loggingFault=true;
  }
  if(logDrops!=reportedLogDrops){reportedLogDrops=logDrops;queueConsole("LOG_QUEUE_OVERFLOW; requests disabled until reset\n");}
  if(loggingFault&&diagnostics.busy())diagnostics.cancel();
  diagnostics.tick(millis());
  if(restartBus){restartBus=false;configureCan(configuredRate,!ready);}

  if(scanning){
    if(scanFrames>=3){
      uint32_t found=scanRates[scanIndex];scanning=false;
      configureCan(found,false);bitrate=found;
      note("AUTO_LOCK","At least 3 valid frames; CAN bitrate detected, not OBD address/protocol");status();
    }else if(uint32_t(millis()-scanStart)>=2000){
      if(++scanIndex>=sizeof(scanRates)/sizeof(scanRates[0])){
        scanning=false;ready=false;bitrate=0;
        note("AUTO_NO_LOCK","No sufficient traffic; remains listen-only; use baud command");
      }else{
        configureCan(scanRates[scanIndex],true);scanFrames=0;scanStart=millis();
        note("AUTO_LISTEN","Next candidate; passive only");
      }
    }
  }
  static uint32_t errorAt=0;
  if(uint32_t(millis()-errorAt)>=100){
    errorAt=millis();CAN_error_t e;
    if(can.error(e,false)){
      char b[100];snprintf(b,sizeof b,"ESR=%08lX ECR=%04X ACK=%u CRC=%u TXerr=%u RXerr=%u",
        (unsigned long)e.ESR1,e.ECR,e.ACK_ERR,e.CRC_ERR,e.TX_ERR_COUNTER,e.RX_ERR_COUNTER);
      note("CAN_ERROR",b);
      if(ready&&((FLEXCANb_ESR1(CAN1)>>4)&3u)>=2u){diagnostics.cancel();ready=false;restartBus=true;}
    }
  }
  serialInput();
  for(unsigned i=0;i<8&&logCount;++i){pumpLog();pumpConsole();}
  pumpConsole();
  if(sdGood&&!diagnostics.busy()&&uint32_t(millis()-flushAt)>=1000){
    logfile.flush();flushAt=millis();
    if(logfile.getWriteError()){sdGood=false;loggingFault=true;queueConsole("SD_FLUSH_FAILED\n");}
  }
}
