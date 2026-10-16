#include <FlexCAN_T4.h>
#include <SD.h>
#include "DiagnosticCore.h"
#include "LogReport.h"
#include "Config.h"
#include "CaptureRing.h"
#include <new>
automatic::Config config;
automatic::Schedule schedule;
bool configGood=false, running=false, finished=false, closing=false, completedClean=false;
uint32_t runStart=0, setupAt=0, summaryAt=0;
uint32_t responseLedAt=0;
bool responseLedSeen=false;
// CAN1 default pins: TX=22, RX=23. TLE9371VSJ STB is grounded externally.
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can;
File logfile;
obd::Address addressing;
uint32_t configuredRate=500000, transaction=0;
bool ready=false, transactionEnded=false, sdGood=false;
bool canStarted=false, finalSummary=false;
volatile bool captureEnabled=false;
uint32_t flushAt=0;

// ISR only copies frames. Never perform SD or USB work inside the interrupt.
struct Captured { CAN_message_t frame; uint32_t ms,us,sequence; };
constexpr uint32_t CAPTURE_CAPACITY=65536;
automatic::CaptureRing<Captured> captureRing;
Captured *captureMemory=nullptr;
volatile uint32_t captureDrops=0, captureSeen=0, capturePeak=0, hardwareOverruns=0;
uint32_t reportedCaptureDrops=0, captureDrained=0;
struct LogLine {char text[320];};
LogLine emergencyLog[64];
LogLine *logQueue=emergencyLog;
unsigned logCapacity=64,logHead=0,logTail=0,logCount=0,logPeak=0;
uint32_t logDrops=0,consoleDrops=0,reportedLogDrops=0;
char sdBatch[8192];size_t sdBatchUsed=0;
uint32_t sdWrites=0,sdWriteMaxUs=0,sdFailures=0,sdBytesUnwritten=0;
char consoleQueue[32768];
unsigned consoleHead=0,consoleTail=0,consoleCount=0;
bool loggingFault=false;
char logName[24]={};
// A visible 150 ms pulse for a validated diagnostic reply, at most twice/second.
// No delay(): CAN receive and SD logging continue while the LED is lit.
void indicateResponse(){
  uint32_t now=millis();
  if(!responseLedSeen||uint32_t(now-responseLedAt)>=500){
    responseLedAt=now;responseLedSeen=true;
  }
}

void reportLine(const char *text){
  if(logCount==logCapacity){++logDrops;loggingFault=true;return;}
  snprintf(logQueue[logHead].text,sizeof(logQueue[logHead].text),"%s\n",text);
  logHead=(logHead+1)%logCapacity;++logCount;if(logCount>logPeak)logPeak=logCount;
}

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
  if(logCount==logCapacity){++logDrops;loggingFault=true;return;}
  obd::formatEvent(logQueue[logHead].text,sizeof(logQueue[logHead].text),
    when,transaction,configuredRate,kind,id,text);
  logHead=(logHead+1)%logCapacity;++logCount;if(logCount>logPeak)logPeak=logCount;
}
void note(const char *kind,const char *text){recordAt(millis(),kind,0,text);}
void raw(const char *kind,const obd::Frame &f,uint32_t when){
  char b[72];int pos=snprintf(b,sizeof b,"%s DLC=%u",f.extended?"EXT":"STD",f.len);
  for(unsigned i=0;i<f.len&&i<8;++i)pos+=snprintf(b+pos,sizeof(b)-pos," %02X",f.data[i]);
  recordAt(when,kind,f.id,b);
}
void writeBatch(){
  if(!sdBatchUsed)return;
  if(sdGood){
    uint32_t start=micros();size_t written=logfile.write((const uint8_t*)sdBatch,sdBatchUsed);
    uint32_t elapsed=micros()-start;++sdWrites;if(elapsed>sdWriteMaxUs)sdWriteMaxUs=elapsed;
    if(written!=sdBatchUsed){
      sdBytesUnwritten+=sdBatchUsed-written;++sdFailures;sdGood=false;loggingFault=true;
      queueConsole("SD_WRITE_FAILED; data incomplete\n");
    }
  }else sdBytesUnwritten+=sdBatchUsed;
  sdBatchUsed=0;
}
void pumpLog(){
  if(!logCount)return;
  const char *line=logQueue[logTail].text;size_t n=strlen(line);
  if(sdBatchUsed+n>sizeof(sdBatch))writeBatch();
  memcpy(sdBatch+sdBatchUsed,line,n);sdBatchUsed+=n;
  queueConsole(line);logTail=(logTail+1)%logCapacity;--logCount;
  if(sdBatchUsed>=4096)writeBatch();
}
void onCan(const CAN_message_t &f){
  if(!captureEnabled)return;
  Captured c;c.frame=f;c.ms=millis();c.us=micros();c.sequence=++captureSeen;
  if(f.flags.overrun)++hardwareOverruns;
  if(!captureRing.push(c)){++captureDrops;return;}
  uint32_t used=captureRing.size();if(used>capturePeak)capturePeak=used;
}
bool popCapture(Captured &c){return captureRing.pop(c);}
bool txPending(){
  for(unsigned i=8;i<16;++i)
    if(FLEXCAN_get_code(FLEXCANb_MBn_CS(CAN1,i))==FLEXCAN_MB_CODE_TX_ONCE)return true;
  return false;
}
// Observe FIFO overflow BEFORE FlexCAN_T4 clears it without setting msg.flags.overrun.
void captureCanInterrupt(){
  if(FLEXCANb_IFLAG1(CAN1)&FLEXCAN_IFLAG1_BUF7I){
    if(captureEnabled)++hardwareOverruns;
    FLEXCANb_IFLAG1(CAN1)=FLEXCAN_IFLAG1_BUF7I;
  }
  flexcan_isr_can1();
}
void configureCan(uint32_t rate,bool listen){
  ready=false;
  can.begin();can.setMaxMB(16);can.setBaudRate(rate,LISTEN_ONLY);
  can.enableFIFO();can.setFIFOFilter(ACCEPT_ALL);
  can.onReceive(onCan);
  attachInterruptVector(IRQ_CAN1,captureCanInterrupt);
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
    if(n&&(!strcmp(kind,"POSITIVE")||!strcmp(kind,"NEGATIVE")||!strcmp(kind,"RESPONSE_PENDING")))indicateResponse();
    if(!n)recordAt(millis(),kind,id,"");
    for(size_t offset=0;offset<n;offset+=24){
      char b[110];int pos=snprintf(b,sizeof b,"len=%u offset=%u",(unsigned)n,(unsigned)offset);
      for(size_t i=offset;i<n&&i<offset+24;++i)pos+=snprintf(b+pos,sizeof(b)-pos," %02X",p[i]);
      recordAt(millis(),kind,id,b);
    }
    if(n>=3 && p[0]==0x7F)recordAt(millis(),"NRC",id,nrcName(p[2]));
    if(!strcmp(kind,"POSITIVE"))obd::reportPositive(reportLine,id,p,n);
    // Diagnostic completion never resets the receive controller.
    if(!strcmp(kind,"REQUEST_END")||!strcmp(kind,"CANCELLED"))transactionEnded=true;
  }
} sink;
obd::DiagnosticCore diagnostics(sink);
void setup(){
  Serial.begin(115200);
  pinMode(LED_BUILTIN,OUTPUT);digitalWrite(LED_BUILTIN,LOW);
  if(SD.begin(BUILTIN_SDCARD)){
    for(unsigned i=0;i<10000;++i){
      snprintf(logName,sizeof logName,"AUTO%04u.TXT",i);
      if(!SD.exists(logName)){logfile=SD.open(logName,FILE_WRITE);break;}
    }
    sdGood=bool(logfile);
  }
  if(sdGood){
    char header[100];obd::formatHeader(header,sizeof header,"");
    queueConsole(header);
    sdGood=logfile.write((const uint8_t*)header,strlen(header))==strlen(header);
  }
  if(!sdGood)queueConsole("SD_INIT_FAILED; requests disabled; use FAT32 SD in Teensy's built-in slot\n");
  note("BOOT","Teensy 4.1 CAN1 TX22 RX23; STB grounded; timestamps=uptime milliseconds");
  if(sdGood){
    File file=SD.open("config.json",FILE_READ);
    const char *reason="Missing config.json";
    if(file&&file.size()<=8192){
      JsonDocument document;
      DeserializationError result=deserializeJson(document,file,DeserializationOption::NestingLimit(4));
      if(result)reason="Invalid JSON syntax";
      else {
        configGood=automatic::parseConfig(document.as<JsonVariantConst>(),config,reason);
        while(file.available()){
          int ch=file.read();
          if(ch!=' '&&ch!='\t'&&ch!='\r'&&ch!='\n'){configGood=false;reason="Trailing data after JSON";break;}
        }
      }
    }else if(file)reason="config.json exceeds 8192 bytes";
    if(file)file.close();
    if(!configGood)note("CONFIG_ERROR",reason);
  }
  if(configGood){
    captureMemory=static_cast<Captured*>(extmem_malloc(sizeof(Captured)*CAPTURE_CAPACITY));
    LogLine *largeLog=static_cast<LogLine*>(extmem_malloc(sizeof(LogLine)*4096));
    // extmem_malloc can fall back to internal RAM: verify actual external addresses.
    bool memoryGood=captureMemory&&largeLog&&
      ((uintptr_t(captureMemory)>>28)==7)&&((uintptr_t(largeLog)>>28)==7);
    if(!memoryGood){
      if(captureMemory)extmem_free(captureMemory);
      if(largeLog)extmem_free(largeLog);
      captureMemory=nullptr;configGood=false;loggingFault=true;
      note("PSRAM_ERROR","External RAM unavailable; no CAN traffic transmitted");
    }else{
      for(uint32_t i=0;i<CAPTURE_CAPACITY;++i)new(captureMemory+i) Captured{};
      for(unsigned i=0;i<4096;++i)new(largeLog+i) LogLine{};
      for(unsigned i=0;i<logCount;++i)largeLog[i]=logQueue[(logTail+i)%logCapacity];
      logQueue=largeLog;logTail=0;logHead=logCount;logCapacity=4096;
      captureRing.begin(captureMemory,CAPTURE_CAPACITY);
      note("BUFFERS","PSRAM RX capacity=65536 frames; log capacity=4096 lines; SD batches >=4096 bytes");
    }
  }
  if(configGood){
    addressing=config.address;configuredRate=config.bitrate;
    diagnostics.setResponseWindow(config.responseTimeout);
    captureEnabled=true;configureCan(config.bitrate,false);canStarted=true;
    note("CAPTURE_START","ACK enabled; diagnostic requests delayed; no resets during polling");
    note("CONFIG_OK","SD configuration accepted; no serial commands; startup delay begins");
    char b[120];snprintf(b,sizeof b,"jobs=%u startup_ms=%lu duration_ms=%lu response_window_ms=%lu TX=%08lX RX=%08lX",
      unsigned(config.count),(unsigned long)config.startup,(unsigned long)config.duration,
      (unsigned long)config.responseTimeout,(unsigned long)addressing.tx,(unsigned long)addressing.rx);
    note("CONFIG",b);
  }
  setupAt=millis();
  if(!configGood||!sdGood)closing=true;
}
void summary(){
  char b[160];
  snprintf(b,sizeof b,"RXdrops=%lu logDrops=%lu consoleDrops=%lu",(unsigned long)captureDrops,(unsigned long)logDrops,(unsigned long)consoleDrops);
  note("STATUS",b);
  snprintf(b,sizeof b,"seen=%lu drained=%lu buffered=%lu peak=%lu capacity=%lu hw_overruns=%lu",
    (unsigned long)captureSeen,(unsigned long)captureDrained,(unsigned long)captureRing.size(),
    (unsigned long)capturePeak,(unsigned long)CAPTURE_CAPACITY,(unsigned long)hardwareOverruns);
  note("CAPTURE_STATUS",b);
  snprintf(b,sizeof b,"log_peak=%u log_capacity=%u write_calls=%lu max_write_us=%lu write_failures=%lu unwritten_bytes=%lu",
    logPeak,logCapacity,(unsigned long)sdWrites,(unsigned long)sdWriteMaxUs,(unsigned long)sdFailures,(unsigned long)sdBytesUnwritten);
  note("SD_STATUS",b);
  for(size_t i=0;i<config.count;++i){auto &j=config.jobs[i];
    snprintf(b,sizeof b,"job=%u mode=%02X interval_ms=%lu submitted=%lu skipped_periods=%lu late_starts=%lu max_lateness_ms=%lu",
      unsigned(i+1),j.bytes[0],(unsigned long)j.interval,(unsigned long)j.sent,(unsigned long)j.skipped,(unsigned long)j.late,(unsigned long)j.maxLate);
    note("SCHEDULE",b);
  }
}
void stopRun(const char *reason){
  if(closing||finished)return;
  if(running){
    uint32_t end=millis();
    if(uint32_t(end-runStart)>=config.duration)end=runStart+config.duration-1;
    schedule.finish(config.jobs,config.count,end);
  }
  running=false;ready=false;diagnostics.cancel();transactionEnded=false;
  noInterrupts();captureEnabled=false;uint32_t boundaryFlags=canStarted?FLEXCANb_IFLAG1(CAN1):0;interrupts();
  if(canStarted){can.disableFIFOInterrupt();can.setBaudRate(configuredRate,LISTEN_ONLY);}
  char boundary[120];snprintf(boundary,sizeof boundary,"fifo_pending=%u; capture ends here; later bus traffic excluded; draining software queue",
    unsigned((boundaryFlags&FLEXCAN_IFLAG1_BUF5I)!=0));note("CAPTURE_STOP",boundary);
  note("STOP",reason);closing=true;
}
void loop(){
  if(finished){if(!completedClean)digitalWrite(LED_BUILTIN,(millis()/250)%2);pumpConsole();return;}
  digitalWrite(LED_BUILTIN,running&&!closing&&responseLedSeen&&uint32_t(millis()-responseLedAt)<150 ? HIGH : LOW);
  Captured c;
  if(configGood){
    // Reserve space for decoded messages; RX remains buffered during slow SD writes.
    for(unsigned i=0;i<128&&logCount<logCapacity-512&&popCapture(c);++i){
      obd::Frame f={};f.id=c.frame.id;f.extended=c.frame.flags.extended;f.len=c.frame.len;
      memcpy(f.data,c.frame.buf,8);
      char details[130];int pos=snprintf(details,sizeof details,"seq=%lu us=%lu %s DLC=%u",
        (unsigned long)c.sequence,(unsigned long)c.us,f.extended?"EXT":"STD",f.len);
      if(!c.frame.flags.remote)for(unsigned j=0;j<f.len&&j<8;++j)pos+=snprintf(details+pos,sizeof(details)-pos," %02X",f.data[j]);
      recordAt(c.ms,c.frame.flags.remote?"RX_REMOTE":"RX",f.id,details);++captureDrained;
      if(c.frame.flags.overrun){note("CAN_OVERRUN","Hardware receive overrun");loggingFault=true;}
      if(!closing&&!c.frame.flags.remote)diagnostics.receive(f,c.ms);
    }
    if(captureDrops!=reportedCaptureDrops){reportedCaptureDrops=captureDrops;note("RX_QUEUE_OVERFLOW","Capture lost");loggingFault=true;}
    if(logDrops!=reportedLogDrops){reportedLogDrops=logDrops;queueConsole("LOG_QUEUE_OVERFLOW\n");}
  }
  if(configGood&&!closing){
    // error() also reports normal TX/RX state transitions: only flag actual error bits/counters.
    CAN_error_t e;
    for(unsigned i=0;i<8&&can.error(e,false);++i){
      if((e.ESR1&0xFC00u)||e.TX_ERR_COUNTER||e.RX_ERR_COUNTER||((e.ESR1>>4)&3u)){
        char b[100];snprintf(b,sizeof b,"ESR=%08lX ECR=%04X TXerr=%u RXerr=%u",(unsigned long)e.ESR1,e.ECR,e.TX_ERR_COUNTER,e.RX_ERR_COUNTER);
        note("CAN_ERROR",b);
      }
    }
    if(hardwareOverruns){loggingFault=true;}
    if(((FLEXCANb_ESR1(CAN1)>>4)&3u)>=2u){note("BUS_OFF","Automatic run stopped");loggingFault=true;}
    if(loggingFault||!sdGood)stopRun("Logging or CAN failure; reset required");
    else {
      if(captureRing.size()==0)diagnostics.tick(millis());
      if(transactionEnded){
        transactionEnded=false;
        if(txPending()){
          loggingFault=true;stopRun("TX pending at transaction end; stopped without resetting RX");
        }
      }
      uint32_t now=millis();
      if(!closing&&!running&&uint32_t(now-setupAt)>=config.startup){
        runStart=millis();running=true;
        schedule.begin(config.jobs,config.count,runStart);note("RUN_START","One transaction at a time; late periods skipped, never replayed");
      }
      now=millis();
      if(running){
        if(uint32_t(now-runStart)>=config.duration){
          // Stop scheduling at the duration boundary; let the active exchange finish (up to 10 s).
          if(!diagnostics.busy())stopRun("Configured duration finished");
        }else if(!diagnostics.busy()&&captureRing.size()==0){
          int index=schedule.select(config.jobs,config.count,now);
          if(index>=0){auto &j=config.jobs[index];++transaction;
            char b[110];snprintf(b,sizeof b,"job=%u target_interval_ms=%lu skipped_total=%lu late_starts=%lu",
              unsigned(index+1),(unsigned long)j.interval,(unsigned long)j.skipped,(unsigned long)j.late);
            note("SCHEDULE_SEND",b);obd::reportRequest(reportLine,j.bytes,j.length,"");
            if(diagnostics.start(j.bytes,j.length,addressing,millis()))++j.sent;
            else {loggingFault=true;stopRun("Request transmission failed");}
          }
        }
      }
      if(running&&uint32_t(now-summaryAt)>=1000){summaryAt=now;summary();}
    }
  }
  if(closing&&captureRing.size()==0&&!finalSummary){
    finalSummary=true;summary();note("CAPTURE_DRAINED","All queued frames processed; final SD flush follows");
  }
  for(unsigned i=0;i<128&&logCount;++i){pumpLog();pumpConsole();}
  pumpConsole();
  if(sdGood&&!diagnostics.busy()&&uint32_t(millis()-flushAt)>=1000){
    writeBatch();logfile.flush();flushAt=millis();
    if(logfile.getWriteError()){++sdFailures;sdGood=false;loggingFault=true;}
  }
  if(closing&&finalSummary&&!logCount){
    writeBatch();
    bool ok=configGood&&sdGood&&!loggingFault;
    if(logfile){logfile.flush();ok=ok&&!logfile.getWriteError();logfile.close();}
    completedClean=ok;sdGood=false;finished=true;digitalWrite(LED_BUILTIN,LOW);
    queueConsole(ok?"LOG_CLOSED; run complete\n":"RUN_FAILED; check SD/log and reset\n");pumpConsole();
  }
}
