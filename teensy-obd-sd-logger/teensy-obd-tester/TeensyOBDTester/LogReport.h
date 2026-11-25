#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace obd {
constexpr const char *LOG_TITLE="OBD-SkillLogger Version 1.0.0";
constexpr size_t DATE_WIDTH=19;
inline bool leap(unsigned y){return y%4==0 && (y%100!=0 || y%400==0);}
inline unsigned monthDays(unsigned y,unsigned m){
  static const unsigned days[]={31,28,31,30,31,30,31,31,30,31,30,31};
  return days[m-1]+(m==2&&leap(y));
}
// Local civil seconds since 2000-01-01. No timezone conversion or guessed RTC.
inline bool parseLocalDate(const char *s,uint32_t &seconds){
  if(strlen(s)!=19||s[4]!='-'||s[7]!='-'||s[10]!=' '||s[13]!=':'||s[16]!=':')return false;
  for(unsigned i=0;i<19;++i)if(i!=4&&i!=7&&i!=10&&i!=13&&i!=16 && (s[i]<'0'||s[i]>'9'))return false;
  unsigned y,m,d,h,min,sec;
  if(sscanf(s,"%4u-%2u-%2u %2u:%2u:%2u",&y,&m,&d,&h,&min,&sec)!=6)return false;
  if(y<2000||y>2099||m<1||m>12||d<1||d>monthDays(y,m)||h>23||min>59||sec>59)return false;
  uint32_t days=0;
  for(unsigned yr=2000;yr<y;++yr)days+=leap(yr)?366:365;
  for(unsigned mo=1;mo<m;++mo)days+=monthDays(y,mo);
  seconds=((days+d-1)*24+h)*3600+min*60+sec;return true;
}
inline void formatLocalDate(uint32_t seconds,char *out,size_t cap){
  uint32_t days=seconds/86400,part=seconds%86400;unsigned y=2000,m=1;
  while(days>=(leap(y)?366u:365u)){days-=leap(y)?366:365;++y;}
  while(days>=monthDays(y,m)){days-=monthDays(y,m);++m;}
  snprintf(out,cap,"%04u-%02u-%02u %02u:%02u:%02u",y%10000,m%100,unsigned(days+1)%100,
    unsigned(part/3600),unsigned(part/60%60),unsigned(part%60));
}
inline void formatHeader(char *out,size_t cap,const char *date){
  // Fixed-width date field permits safe in-place update after PC time is entered.
  snprintf(out,cap,"%s\nDate: %-19.19s\n\n",LOG_TITLE,date?date:"");
}
inline void formatEvent(char *out,size_t cap,uint32_t ms,uint32_t request,
                        uint32_t rate,const char *kind,uint32_t id,const char *detail){
  snprintf(out,cap,"[%10lu ms] Request %-4lu CAN %08lX  %-22s %s (bitrate %lu)\n",
    (unsigned long)ms,(unsigned long)request,(unsigned long)id,kind,detail?detail:"",
    (unsigned long)rate);
}
inline const char *modeName(unsigned mode){
  switch(mode){
    case 1:return "Current powertrain diagnostic data";
    case 2:return "Freeze-frame data";
    case 3:return "Stored emission-related trouble codes";
    case 4:return "Clear/reset diagnostic information";
    case 5:return "Oxygen sensor monitoring";
    case 6:return "On-board monitoring test results";
    case 7:return "Pending emission-related trouble codes";
    case 8:return "Control of on-board system/test/component";
    case 9:return "Vehicle information";
    case 10:return "Permanent emission-related trouble codes";
    default:return "Unknown mode";
  }
}
inline const char *parameterName(unsigned mode,unsigned pid){
  if(pid<=0xE0 && pid%32==0)return "Supported identifier bitmap";
  if(mode==9){
    switch(pid){case 2:return "Vehicle identification number (VIN)";
      case 4:return "Calibration identification";case 6:return "Calibration verification numbers";
      case 10:return "ECU name";default:return "Vehicle information identifier";}
  }
  if(mode==6)return "Monitor identifier (MID)";
  if(mode!=1&&mode!=2)return "Service-specific identifier";
  switch(pid){
    case 1:return "Monitor status since DTCs cleared";case 2:return "Freeze-frame trigger DTC";
    case 4:return "Calculated load";case 5:return "Engine coolant temperature";
    case 0x0B:return "Intake manifold absolute pressure";case 0x0C:return "Engine RPM";
    case 0x0D:return "Vehicle speed";case 0x0F:return "Intake air temperature";
    case 0x10:return "Mass air flow";case 0x11:return "Throttle position";
    case 0x42:return "Control module voltage";default:return "Parameter (definition not installed)";
  }
}
typedef void (*ReportLine)(const char *);
inline void reportRequest(ReportLine emit,const uint8_t *p,size_t n,const char *date){
  char b[160];emit("");snprintf(b,sizeof b,"Mode %02X - %s",p[0],modeName(p[0]));emit(b);
  emit("----------------------------------------------------------------");
  if(date&&*date){snprintf(b,sizeof b,"Request time: %s",date);emit(b);}
  size_t pos=snprintf(b,sizeof b,"Request:");
  for(size_t i=0;i<n;++i)pos+=snprintf(b+pos,sizeof(b)-pos," %02X",p[i]);
  emit(b);
}
inline void reportPositive(ReportLine emit,uint32_t id,const uint8_t *p,size_t n){
  if(!n||p[0]<0x41||p[0]>0x4A)return;
  const unsigned mode=p[0]-0x40;char b[160];
  snprintf(b,sizeof b,"ECU: %08lX",(unsigned long)id);emit(b);
  if((mode==1||mode==2||mode==6||mode==9)&&n>=2){
    const unsigned pid=p[1],offset=mode==2?3:2;
    if(mode==2&&n>=3){snprintf(b,sizeof b,"Freeze frame: %02X",p[2]);emit(b);}
    if(pid<=0xE0&&pid%32==0&&n>=offset+4){
      uint32_t mask=(uint32_t(p[offset])<<24)|(uint32_t(p[offset+1])<<16)|
        (uint32_t(p[offset+2])<<8)|p[offset+3];
      emit("List of supported identifiers (this ECU and returned range only)");
      emit("Mode    PID/MID/INFO    Module      Comment");
      for(unsigned bit=0;bit<32;++bit)if(mask&(uint32_t(1)<<(31-bit))){
        unsigned supported=pid+bit+1;
        snprintf(b,sizeof b,"%02X      %02X              %08lX    %s",mode,supported,
          (unsigned long)id,parameterName(mode,supported));emit(b);
      }
      if(!mask)emit("No identifiers advertised in this range.");
      return;
    }
    if((mode==1||mode==2)&&n>offset){
      const uint8_t *d=p+offset;size_t count=n-offset;double v=0;const char *unit=nullptr;
      switch(pid){
        case 4:case 0x11:v=d[0]*100.0/255;unit="%";break;
        case 5:case 0x0F:v=int(d[0])-40;unit="C";break;
        case 0x0B:v=d[0];unit="kPa";break;
        case 0x0D:v=d[0];unit="km/h";break;
        case 0x0C:if(count>=2){v=((d[0]<<8)|d[1])/4.0;unit="rpm";}break;
        case 0x10:if(count>=2){v=((d[0]<<8)|d[1])/100.0;unit="g/s";}break;
        case 0x42:if(count>=2){v=((d[0]<<8)|d[1])/1000.0;unit="V";}break;
      }
      if(unit){snprintf(b,sizeof b,"PID %02X    %.3f %s    %s",pid,v,unit,parameterName(mode,pid));emit(b);return;}
    }
    // CAN Mode 09 has a count byte before these ASCII information records.
    if(mode==9&&(pid==2||pid==10)&&n>3){
      if(pid==2&&(p[2]!=1||n!=20)){emit("VIN layout not recognized; see raw response.");return;}
      char text[81];size_t count=n-3;if(count>80)count=80;
      for(size_t i=0;i<count;++i)text[i]=(p[i+3]>=32&&p[i+3]<=126)?char(p[i+3]):'.';
      text[count]=0;snprintf(b,sizeof b,"INFO %02X    %s    %s",pid,text,parameterName(mode,pid));emit(b);return;
    }
    snprintf(b,sizeof b,"Identifier %02X - %s; see raw response.",pid,parameterName(mode,pid));emit(b);return;
  }
  if(mode==3||mode==7||mode==10){
    // ISO 15765 OBD DTC response: service, count, then two bytes per DTC.
    if(n<2||n<2+size_t(p[1])*2){emit("Incomplete DTC list; see raw response.");return;}
    for(size_t i=2+size_t(p[1])*2;i<n;++i)if(p[i]){
      emit("DTC count does not match data; see raw response.");return;
    }
    snprintf(b,sizeof b,"Fault code entries: %u",p[1]);emit(b);
    if(!p[1])emit("No fault code entry.");
    for(unsigned i=0;i<p[1];++i){
      uint8_t a=p[2+2*i],c=p[3+2*i];
      if(!a&&!c){emit("Empty DTC entry in counted list; see raw response.");continue;}
      snprintf(b,sizeof b,"%c%u%01X%02X    Description not installed","PCBU"[a>>6],
        (a>>4)&3,a&15,c);emit(b);
    }
    return;
  }
  if(mode==4){emit("ECU acknowledged clear/reset request.");return;}
  emit("Positive response; see raw payload for service-specific data.");
}
} // namespace obd
